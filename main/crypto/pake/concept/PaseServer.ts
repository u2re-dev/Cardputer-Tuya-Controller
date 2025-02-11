/**
 * @license
 * Copyright 2022-2025 Matter.js Authors
 * SPDX-License-Identifier: Apache-2.0
 */

import {
    Bytes,
    Crypto,
    ec,
    Logger,
    MatterFlowError,
    PbkdfParameters,
    Spake2p,
    Time,
    Timer,
    UnexpectedDataError,
} from "#general";
import { SessionManager } from "#session/SessionManager.js";
import { NodeId, ProtocolStatusCode, SECURE_CHANNEL_PROTOCOL_ID } from "#types";
import { MessageExchange } from "../../protocol/MessageExchange.js";
import { ProtocolHandler } from "../../protocol/ProtocolHandler.js";
import { ChannelStatusResponseError } from "../../securechannel/SecureChannelMessenger.js";
import { DEFAULT_PASSCODE_ID, PaseServerMessenger, SPAKE_CONTEXT } from "./PaseMessenger.js";

const { bytesToNumberBE } = ec;

const logger = Logger.get("PaseServer");

const PASE_PAIRING_TIMEOUT_MS = 60_000;
const PASE_COMMISSIONING_MAX_ERRORS = 20;

export class MaximumPasePairingErrorsReachedError extends MatterFlowError {}

export class PaseServer implements ProtocolHandler {
    private pairingTimer: Timer | undefined;
    private pairingErrors = 0;

    static async fromPin(sessions: SessionManager, setupPinCode: number, pbkdfParameters: PbkdfParameters) {
        const { w0, L } = await Spake2p.computeW0L(pbkdfParameters, setupPinCode);
        return new PaseServer(sessions, w0, L, pbkdfParameters);
    }

    static fromVerificationValue(
        sessions: SessionManager,
        verificationValue: Uint8Array,
        pbkdfParameters?: PbkdfParameters,
    ) {
        const w0 = bytesToNumberBE(verificationValue.slice(0, 32));
        const L = verificationValue.slice(32, 32 + 65);
        return new PaseServer(sessions, w0, L, pbkdfParameters);
    }

    constructor(
        private sessions: SessionManager,
        private readonly w0: bigint,
        private readonly L: Uint8Array,
        private readonly pbkdfParameters?: PbkdfParameters,
    ) {}

    getId(): number {
        return SECURE_CHANNEL_PROTOCOL_ID;
    }

    async onNewExchange(exchange: MessageExchange) {
        const messenger = new PaseServerMessenger(exchange);
        try {
            await this.handlePairingRequest(messenger);
        } catch (error) {
            this.pairingErrors++;
            logger.error(
                `An error occurred during the PASE commissioning (${this.pairingErrors}/${PASE_COMMISSIONING_MAX_ERRORS}):`,
                error,
            );

            // If we received a ChannelStatusResponseError we do not need to send one back, so just cancel pairing
            const sendError = !(error instanceof ChannelStatusResponseError);
            await this.cancelPairing(messenger, sendError);

            if (this.pairingErrors >= PASE_COMMISSIONING_MAX_ERRORS) {
                throw new MaximumPasePairingErrorsReachedError(
                    `Pase server: Too many errors during PASE commissioning, aborting commissioning window`,
                );
            }
        } finally {
            // Destroy the unsecure session used to establish the secure Case session
            await exchange.session.destroy();
        }
    }

    private async handlePairingRequest(messenger: PaseServerMessenger) {
        // When a Commissioner is either in the process of establishing a PASE session with the Commissionee or has
        // successfully established a session, the Commissionee SHALL NOT accept any more requests for new PASE
        // sessions until session establishment fails or the successfully established PASE session is terminated on
        // the commissioning channel.
        if (this.sessions.getPaseSession()) {
            throw new MatterFlowError(
                "Pase server: Pairing already in progress (PASE session exists), ignoring new exchange.",
            );
        }

        if (this.pairingTimer !== undefined && this.pairingTimer.isRunning) {
            throw new MatterFlowError(
                "Pase server: Pairing already in progress (PASE establishment Timer running), ignoring new exchange.",
            );
        }

        logger.info(`Received pairing request from ${messenger.getChannelName()}.`);

        this.pairingTimer = Time.getTimer("PASE pairing timeout", PASE_PAIRING_TIMEOUT_MS, () =>
            this.cancelPairing(messenger),
        ).start();

        // Read pbkdfRequest and send pbkdfResponse
        const {
            requestPayload,
            request: {
                initiatorRandom,
                initiatorSessionParams,
                passcodeId,
                hasPbkdfParameters,
                initiatorSessionId: peerSessionId,
            },
        } = await messenger.readPbkdfParamRequest();
        if (passcodeId !== DEFAULT_PASSCODE_ID) {
            throw new UnexpectedDataError(`Unsupported passcode ID ${passcodeId}.`);
        }

        const responderSessionId = await this.sessions.getNextAvailableSessionId(); // Responder Session Id
        const responderRandom = Crypto.getRandom();

        const responsePayload = await messenger.sendPbkdfParamResponse({
            initiatorRandom,
            responderRandom,
            responderSessionId,
            pbkdfParameters: hasPbkdfParameters ? undefined : this.pbkdfParameters,
            responderSessionParams: this.sessions.sessionParameters, // responder session parameters
        });

        // Process pake1 and send pake2
        const spake2p = Spake2p.create(Crypto.hash([SPAKE_CONTEXT, requestPayload, responsePayload]), this.w0);
        const { x: X } = await messenger.readPasePake1();
        const Y = spake2p.computeY();
        const { Ke, hAY, hBX } = await spake2p.computeSecretAndVerifiersFromX(this.L, X, Y);
        await messenger.sendPasePake2({ y: Y, verifier: hBX });

        // Read and process pake3
        const { verifier } = await messenger.readPasePake3();
        if (!Bytes.areEqual(verifier, hAY)) {
            throw new UnexpectedDataError("Received incorrect key confirmation from the initiator.");
        }

        // All good! Creating the secure PASE session
        await this.sessions.createSecureSession({
            sessionId: responderSessionId,
            fabric: undefined,
            peerNodeId: NodeId.UNSPECIFIED_NODE_ID,
            peerSessionId,
            sharedSecret: Ke,
            salt: new Uint8Array(0),
            isInitiator: false,
            isResumption: false,
            peerSessionParameters: initiatorSessionParams,
        });
        logger.info(`Session ${responderSessionId} created with ${messenger.getChannelName()}.`);

        await messenger.sendSuccess();
        await messenger.close();

        this.pairingTimer?.stop();
        this.pairingTimer = undefined;
    }

    async cancelPairing(messenger: PaseServerMessenger, sendError = true) {
        this.pairingTimer?.stop();
        this.pairingTimer = undefined;

        if (sendError) {
            await messenger.sendError(ProtocolStatusCode.InvalidParam);
        }
        await messenger.close();
    }

    async close() {
        // Nothing to do
    }
}
