#pragma once
#ifndef ECC_HPP
#define ECC_HPP

//
#include "../../std/math.hpp"
#include <iostream>



//?======================================================
//? Curves Predeclated

#define USE_CURVE_P256
// #define USE_BLS12_381
// #define USE_CURVE_SECP256K1
// #define USE_CURVE_P521

//
static const BigInt B256 = (BigInt(1) << 256);

/**
 * @struct CurveParameters
 * @brief Holds the parameters of an elliptic curve.
 *
 * This structure stores the essential parameters that define an elliptic curve over a finite field.
 * These include the coefficients 'a' and 'b' of the elliptic curve equation y^2 = x^3 + ax + b,
 * the prime 'p' defining the finite field size, the coordinates 'Gx' and 'Gy' of the generator
 * point, and 'n' which is the order of the group generated by the generator point.
 */
struct CurveParameters {
    // defaultly USE_CURVE_P256
    BigInt a  ;//= BigInt("ffffffff00000001000000000000000000000000fffffffffffffffffffffffc", 16);  ///< Coefficient a of the elliptic curve equation.
    BigInt b  ;//= BigInt("5ac635d8aa3a93e7b3ebbd55769886bc651d06b0cc53b0f63bce3c3e27d2604b", 16);  ///< Coefficient b of the elliptic curve equation.
    BigInt p  ;//= BigInt("ffffffff00000001000000000000000000000000ffffffffffffffffffffffff", 16); //B256 - BigInt("1000003d1", 16);  ///< Prime number defining the field size.
    BigInt Gx ;//= BigInt("6b17d1f2e12c4247f8bce6e563a440f277037d812deb33a0f4a13945d898c296", 16); ///< x-coordinate of the generator point.
    BigInt Gy ;//= BigInt("4fe342e2fe1a7f9b8ee7eb4a7c0f9e162bce33576b315ececbb6406837bf51f5", 16); ///< y-coordinate of the generator point.
    BigInt n  ;//= BigInt("ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551", 16); //B256 - BigInt("14551231950b75fc4402da1732fc9bebf", 16);  ///< Order of the group generated by the generator point.
};



//?======================================================
//? ECC Point Class Declaration

/**
 * @class Ecc_Point
 * @brief Represents a point on an elliptic curve.
 *
 * The Ecc_Point class encapsulates a point on an elliptic curve defined over a finite field.
 * It provides functionalities for elliptic curve arithmetic like point addition,
 * negation, and scalar multiplication.
 */
class Ecc_Point {
public:
    //?======================================================
    //? Curve Specific

    static const Ecc_Point ZERO = Ecc_Point(0, 1, 0);
    static const Ecc_Point BASE;

    /**
     * @brief Retrieves a constant reference to the curve parameters.
     *
     * This static function provides access to the elliptic curve parameters used by all instances of Ecc_Point.
     * It ensures that the curve parameters are initialized only once and then reused for any subsequent calls,
     * improving efficiency and consistency across Ecc_Point instances.
     *
     * The function works by creating a temporary Ecc_Point object to trigger the initialization of curve parameters
     * within the constructor of Ecc_Point. These parameters are then copied to a static CurveParameters variable,
     * which is returned by reference to the caller.
     *
     * @note This function is thread-safe only if the compiler guarantees the thread-safe initialization of local static variables.
     *
     * @return A constant reference to the statically stored CurveParameters object, ensuring that all Ecc_Point instances
     * share the same set of elliptic curve parameters.
     */

    //
    static CurveParameters GetCurveParameters();
    static Ecc_Point getM() { return Ecc_Point::fromHex("02886e2f97ace46e55ba9dd7242579f2993b64e16ef3dcab95afd497333d8fa12f"); }
    static Ecc_Point getN() { return Ecc_Point::fromHex("03d8bbd6c639c62937b04d997f38c3770719c629d7014d49a24b4f98baa1292b49"); }
    static Ecc_Point getBase() { return Ecc_Point::BASE; }
    static BigInt getCurveOrder() { return BigInt("ffffffff00000000ffffffffffffffffbce6faada7179e84f3b9cac2fc632551", 16); }
    static BigInt getP() const { return GetCurveParameters().p; }


    //?======================================================
    //? Constructors Specific

    /**
     * @brief Default constructor to initialize an Ecc_Point with BigInt coordinates. Initializes with base points as x and y coordinate.
     * @param x The x-coordinate of the Ecc_Point.
     * @param y The y-coordinate of the Ecc_Point.
     */
    Ecc_Point() : xCoord(BigInt(static_cast<unsigned long int>(0))), yCoord(BigInt(static_cast<unsigned long int>(0))), isInfinity(true) {}

    /**
     * @brief Constructor to initialize an Ecc_Point with BigInt coordinates.
     * @param x The x-coordinate of the Ecc_Point.
     * @param y The y-coordinate of the Ecc_Point.
     */
    Ecc_Point(const BigInt& x, const BigInt& y, const BigInt& z = 1) : xCoord(x), yCoord(y), yCoord(z), isInfinity(false) { setCurveParameters(); }

    /**
     * @brief Copy constructor.
     * @param other The Ecc_Point to copy.
     */
    Ecc_Point(const Ecc_Point& other) : xCoord(other.xCoord), yCoord(other.yCoord), isInfinity(other.isInfinity), curveParams(other.curveParams) {}


    //?======================================================
    //? Operators

    /**
     * @brief Overloads the == operator to compare two Ecc_Points.
     *
     * This function checks if two points on the elliptic curve are equal. Two points
     * P = (x1, y1) and Q = (x2, y2) are considered equal if and only if x1 ≡ x2 and
     * y1 ≡ y2. This comparison is crucial in many cryptographic algorithms to determine
     * if two points are the same.
     *
     * The equality check is performed under modulo p arithmetic for a prime field.
     *
     * @param other The Ecc_Point to compare with this point.
     * @return True if the points are equal (both x and y coordinates match), false otherwise.
     */
    bool operator==(const Ecc_Point& other) const {
        if (isInfinity && other.isInfinity) return true;
        if (isInfinity || other.isInfinity) return false;
        return (xCoord == other.xCoord) && (yCoord == other.yCoord) && (zCoord == other.zCoord);
    }

    /**
     * @brief Overloads the + operator for adding two Ecc_Points.
     *
     * This function implements the addition of two points on an elliptic curve.
     * Given two points P = (x1, y1) and Q = (x2, y2), it computes the resulting
     * point R = P + Q = (x3, y3) using elliptic curve addition formulas.
     *
     * The addition is defined as follows:
     * 1. If P = Q, then the point doubling formula is applied.
     * 2. If P ≠ Q, the slope λ = (y2 - y1) / (x2 - x1) is computed, followed by
     *    x3 = λ² - x1 - x2 and y3 = λ(x1 - x3) - y1.
     * 3. Special cases, like handling the point at infinity, are also considered.
     *
     * Operations are performed under modulo p arithmetic for a prime field.
     *
     * @param other The Ecc_Point to add to this point.
     * @return Ecc_Point representing the sum of this point and the other point.
     */
    Ecc_Point operator+(const Ecc_Point& other) const {
        if (this->isInfinity) return other;
        if (other.isInfinity) return *this;
        if (*this == -other) return Ecc_Point();
        if (*this == other) { return this->doublePoint(); }

        //
        BigInt lambda = (other.yCoord - yCoord) * (inv(other.xCoord - xCoord, curveParams.p));
        BigInt x3 = (lambda * lambda - xCoord - other.xCoord) % curveParams.p;
        BigInt y3 = (lambda * (xCoord - x3) - yCoord) % curveParams.p;
        return Ecc_Point(x3, y3);
    }

    /**
     * @brief Overloads the - operator to negate this Ecc_Point.
     *
     * Negating a point P = (x, y) on the elliptic curve results in the point -P = (x, -y).
     * In the context of modulo arithmetic for a prime field, -y is computed as p - y.
     *
     * This operation is useful for elliptic curve point subtraction, as P - Q is equivalent
     * to P + (-Q).
     *
     * @return Ecc_Point representing the negation of this point on the elliptic curve.
     */
    Ecc_Point operator-() const {
        if (this->isInfinity) return *this;
        return Ecc_Point(xCoord, curveParams.p - yCoord);
    }

    /**
     * @brief Overloads the * operator for scalar multiplication of this Ecc_Point.
     *
     * Scalar multiplication is a fundamental operation in elliptic curve cryptography.
     * It computes kP for a point P on the curve and a scalar k. This operation is
     * equivalent to adding P to itself k times.
     *
     * The scalar multiplication is performed using the "double-and-add" method, which
     * is efficient and reduces the number of elliptic curve operations needed.
     *
     * @param scalar The BigInt scalar to multiply this point by.
     * @return Ecc_Point resulting from the scalar multiplication of this point by the scalar.
     */
    Ecc_Point operator*(const BigInt& scalar) const {
        if (scalar.isZero() || this->isInfinity) { return Ecc_Point(); }
        Ecc_Point result;
        Ecc_Point point = *this;

        BigInt k = scalar;
        while (k > BigInt(static_cast<unsigned long int>(0))) {
            if (k % BigInt(static_cast<unsigned long int>(2)) != BigInt(static_cast<unsigned long int>(0))) {
                result = result + point;
            }
            point = point.doublePoint();
            k /= BigInt(static_cast<unsigned long int>(2));
        }
        return result;
    }

    /**
     * @brief Assignment operator.
     * @param other The Ecc_Point to assign.
     * @return Reference to this Ecc_Point after assignment.
     */
    Ecc_Point& operator=(const Ecc_Point& other) {
        if (this != &other) {
            xCoord = other.xCoord;
            yCoord = other.yCoord;
            isInfinity = other.isInfinity;
            curveParams = other.curveParams;
        }
        return *this;
    }



    //?======================================================
    //? Operations

    //
    static Ecc_Point fromAffine(const AffinePoint &pt);

    //
    AffinePoint toAffine() const;

    //
    Ecc_Point multiply(const BigInt &scalar) const;

    //
    bool equals(const Ecc_Point &other) const;


    /**
     * @brief Doubles this Ecc_Point on the elliptic curve.
     *
     * This function implements the point doubling operation for elliptic curves
     * in Weierstrass form. It calculates 2P = (x', y') for the current point P = (x, y).
     * The doubling is done using the formulas: λ = (3x² + a) / 2y, x' = λ² - 2x,
     * and y' = λ(x - x') - y, where 'a' is the curve parameter and operations are
     * performed under modulo p arithmetic, with p being the prime order of the field.
     * The point P is represented by the member variables xCoord and yCoord.
     *
     * @note Assumes the point is not at infinity and y-coordinate is not zero.
     * Additional handling is required for these special cases.
     *
     * @return Ecc_Point representing the doubled point 2P on the elliptic curve.
     */
    Ecc_Point doublePoint() const;

    /**
     * @brief Get the x-coordinate of the Ecc_Point.
     * @return The x-coordinate.
     */
    const BigInt& getX() const { return xCoord; }

    /**
     * @brief Get the y-coordinate of the Ecc_Point.
     * @return The y-coordinate.
     */
    const BigInt& getY() const { return yCoord; }

    /**
     * @brief Set the x-coordinate of the Ecc_Point.
     * @param x The new x-coordinate.
     */
    void setX(const BigInt& x) { xCoord = x; }

    /**
     * @brief Set the y-coordinate of the Ecc_Point.
     * @param y The new y-coordinate.
     */
    void setY(const BigInt& y) { yCoord = y; }




    //?======================================================
    //? Debug and Bytes operation (conversion)

    /**
     * @brief Prints the coordinates of the Ecc_Point.
     *
     * This function displays the x and y coordinates of the Ecc_Point. If the point
     * is at infinity, it prints a message indicating so. This is useful for debugging
     * and verifying the values of the Ecc_Point during computations.
     */
    void print() const;



    // Парсинг точки из hex‑строки (поддерживается как сжатый, так и несжатый формат).
    // Для сжатого формата: 33 байта (первый байт – 0x02 или 0x03, далее 32 байта x).
    static Ecc_Point fromHex(const std::string &hexStr);
    static std::string n2h(const BigInt &num);
    Bytes toRawBytes(bool isCompressed = true) const;
    Ecc_Point assertValidity() const;

    //
    bool isInfinity;
private:
    CurveParameters curveParams;
    BigInt xCoord; ///< The x-coordinate of the Ecc_Point.
    BigInt yCoord; ///< The y-coordinate of the Ecc_Point.
    BigInt zCoord = 1;

    /**
     * @brief Sets the parameters of the elliptic curve based on the selected curve type.
     *
     * This function initializes the curve parameters (a, b, p, Gx, Gy, n) of the Ecc_Point
     * object based on a predefined set of well-known elliptic curves. The specific curve
     * is selected using preprocessor directives at compile time. This function is invoked
     * during the construction of an Ecc_Point object to ensure it is configured with the
     * correct parameters for the chosen elliptic curve.
     *
     * @note The curve parameters must be predefined and correctly set for the specified
     *       elliptic curve. If no curve is defined, a compile-time error is generated.
     */
    void setCurveParameters();
};

//
using ECCPoint = Ecc_Point;
#endif // ECC_HPP
