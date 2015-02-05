static char RCSId[] =
    "$Id: bigint.cpp,v 1.16 2015/01/27 20:40:23 David Exp David $";
#include <iostream>
#include <vector>
#include <stack>
#include "ydebug.hpp"	// David's debugging macros, used in main() test program
typedef unsigned long long CHUNK;
typedef unsigned long long ULONG;
class BigInteger
{
    // stream input and output
    friend std::ostream& operator<< (std::ostream &strm, BigInteger bigI);
    friend std::istream& operator>> (std::istream &strm, BigInteger &bigI);

    private:
    std::vector<CHUNK> magnitude;	// each element is considered a 'digit'

    enum Sign {
	Positive,
	Negative
    } sign;

    enum NumericConstant {
	Arg1Smaller = -1,
	ArgsEqual = 0,
	Arg1Larger = 1,
	OrEquals,
	AndEquals
    };

    static const unsigned BitsPerByte = 8;
    static const CHUNK HIGH_BIT = 1LL << (sizeof(CHUNK)*BitsPerByte - 1);
    static const CHUNK LOW_BIT = 1;


    /********************************
     * return true if value is zero *
     ********************************/
    bool isZero(const BigInteger &arg) const
    {
	return arg.magnitude.size() == 1 && arg.magnitude[0] == 0;
    }

    /******************************************************
     * flip sign from positive to negative, or vice versa *
     ******************************************************/
    void flipSign(BigInteger &arg) const
    {
	if (isZero(arg))
	    arg.sign = Positive;	// ensure zero is always positive
	else if (arg.sign == Positive)
	    arg.sign = Negative;
	else
	    arg.sign = Positive;
    }

    /*******************************
     * set value of 'this' to zero *
     *******************************/
    void makeZero ()
    {
	this->magnitude.clear();
	this->magnitude.push_back(0);
	this->sign = Positive;	// zero is always positive
    }

    /************************************************************************
     * leading zero values are trailing zeroes in the vector representation *
     ************************************************************************/
    void popLeadingZeros()
    {
	while (this->magnitude.size() > 1 && this->magnitude.back() == 0)
	    this->magnitude.pop_back();
	if (this->magnitude.size() == 1 && this->magnitude[0] == 0)
	    this->sign = Positive;
    }

    /***********************************************************
     * subtract the magnitude of another BigInteger from *this *
     * assumption: magnitude(this) >= magnitude(other)         *
     * all callers of this routine should ensure this is true  *
     ***********************************************************/
    void subtractMagnitude(const BigInteger other)
    {
	ULONG i;
	CHUNK digit;
	ULONG mySize = this->magnitude.size();
	ULONG otherSize = other.magnitude.size();
	ULONG commonSize = otherSize;	// assured by the assumption

	// 'if' depends on the assumption above, other <= this
	if (isZero(other) || isZero(*this))
		return;

	// subtract 'digits' in common positions
	int inBorrow = 0;
	int outBorrow;

	for (i = 0; i < commonSize; ++i) {
	    digit = this->magnitude[i];
	    outBorrow = digit < other.magnitude[i];	// 0(false) or 1(true)
	    digit -= other.magnitude[i];
	    outBorrow |= (digit == 0 && inBorrow);	// borrow gen'd borrow?
	    digit -= inBorrow;
	    this->magnitude[i] = digit;
	    inBorrow = outBorrow;
	}

	for (; i < mySize && inBorrow; ++i) {	// propagate any borrow bit
	    digit = this->magnitude[i];
	    inBorrow = (digit == 0);
	    this->magnitude[i] = digit - 1;
	}

	popLeadingZeros();
	// no overflow possible, due to assumption
    }

    /****************************************************
     * add the magnitude of another BigInteger to *this *
     ****************************************************/
    void addMagnitude(const BigInteger other)
    {
	ULONG commonSize;
	ULONG i;
	CHUNK digit;
	ULONG mySize = this->magnitude.size();
	ULONG otherSize = other.magnitude.size();

	if (isZero(other))
	    return;	// nothing to do
	if (isZero(*this)) {
	    *this = other;
	    return;
	}

	if (otherSize > mySize) {
	    commonSize = mySize;
	    for (i = mySize; i < otherSize; ++i)	// copy high-order
		this->magnitude.push_back(other.magnitude[i]);
	    mySize = otherSize;				// size has changed
	} else
	    commonSize = otherSize;

	// add 'digits' in common positions
	int inCarry = 0;
	int outCarry;

	for (i = 0; i < commonSize; ++i) {
	    digit = this->magnitude[i] + other.magnitude[i];
	    outCarry = (digit < this->magnitude[i]);	// 0(false) or 1(true)
	    digit += inCarry;
	    inCarry = outCarry || ((digit == 0) && inCarry); // carry gen carry?
	    this->magnitude[i] = digit;
	}

	for (; i < mySize && inCarry; ++i) {	// propagate any carry bit
	    digit = this->magnitude[i] += 1;
	    inCarry = (digit == 0);
	    this->magnitude[i] = digit;
	}

	if (i >= mySize && inCarry)		// did we overflow *this?
	    this->magnitude.push_back(1);
    }

    /***************************************************************
     * multiply two big integers and return the result             *
     * see: https://en.wikipedia.org/wiki/Multiplication_algorithm *
     ***************************************************************/
    BigInteger binaryMultiply(const BigInteger multiplicand,
			      const BigInteger multiplier)
    const
    {
	ULONG numBits =
	    multiplier.magnitude.size() * sizeof(CHUNK) * BitsPerByte;
	BigInteger addend;
	BigInteger mask = 1;
	BigInteger product(0);
	ULONG i;
	static const BigInteger bigZero(0);
	static const BigInteger bigOne(1);

	if (isZero(multiplier) || isZero(multiplicand))
	    product = bigZero;
	else if (multiplier == bigOne)
	    product = multiplicand;
	else if (multiplicand == bigOne)
	    product = multiplier;
	else {
	    addend = multiplicand;
	    product = bigZero;

	    for (i = 0; i < numBits; ++i) {
		if ((multiplier & mask) != bigZero)
		    product += addend;
		mask.shiftMeLeft1();
		addend.shiftMeLeft1();
	    }
	}

	if (product != bigZero) {
	    if (multiplier.sign == multiplicand.sign)
		product.sign = Positive;
	    else
		product.sign = Negative;
	}

	return product;
    }

    /*********************************************************
     * divide two big integers and return the result         *
     * see: https://en.wikipedia.org/wiki/Division_algorithm *
     *********************************************************/
    BigInteger binaryDivide(const BigInteger dividend,
	const BigInteger divisor, BigInteger &remainder)
    const
    {
	ULONG numBits = dividend.magnitude.size() * sizeof(CHUNK) * BitsPerByte;
	ULONG i;
	static const BigInteger bigZero(0);
	static const BigInteger bigOne(1);

	if (isZero(divisor))
	    throw("divide by zero");

	remainder = bigZero;

	if (isZero(dividend))
	    return bigZero;

	if (dividend == divisor)
	    return bigOne;

	BigInteger quotient(0);
	BigInteger mask(bigOne << numBits);

	for (i = numBits; i > 0; --i) {
	    remainder.shiftMeLeft1();
	    mask.shiftMeRight1();
	    if (! isZero(dividend & mask))
		remainder |= bigOne;
	    if (remainder >= divisor) {
		remainder -= divisor;
		quotient |= mask;
		//quotient |= (bigOne << (i-1));
	    }
	}

	if (quotient != bigZero) {
	    if (dividend.sign == divisor.sign)
		quotient.sign = Positive;
	    else
		quotient.sign = Negative;
	}
	return quotient;
    }

    /***************************************************
     * set *this to a BigInteger derived from a string *
     ***************************************************/
    void strToBigInteger(std::string numStr, int radix, int start, Sign signArg)
    {
	static const BigInteger bigZero(0);
	static const BigInteger bigOne(1);
	static const BigInteger bigTwo(2);
	static const BigInteger bigThree(3);
	static const BigInteger bigFour(4);
	static const BigInteger bigFive(5);
	static const BigInteger bigSix(6);
	static const BigInteger bigSeven(7);
	static const BigInteger bigEight(8);
	static const BigInteger bigNine(9);
	static const BigInteger bigTen(10);
	static const BigInteger bigEleven(11);
	static const BigInteger bigTwelve(12);
	static const BigInteger bigThirteen(13);
	static const BigInteger bigFourteen(14);
	static const BigInteger bigFifteen(15);

	unsigned index = start;
	BigInteger digit;
	BigInteger bigRadix(radix);

	makeZero();	// clear current value, if any

	unsigned len = numStr.size();

	bool numStrValid = true;
	for (; index < len; ++index) {
	    switch (numStr[index]) {
	    case '0':	digit = bigZero;
			break;
	    case '1':	digit = bigOne;
			break;
	    case '2':	digit = bigTwo;
			break;
	    case '3':	digit = bigThree;
			break;
	    case '4':	digit = bigFour;
			break;
	    case '5':	digit = bigFive;
			break;
	    case '6':	digit = bigSix;
			break;
	    case '7':	digit = bigSeven;
			break;
	    case '8':	digit = bigEight;
			if (radix == 8)
			    numStrValid = false;
			break;
	    case '9':	digit = bigNine;
			if (radix == 8)
			    numStrValid = false;
			break;
	    case 'a':
	    case 'A':	digit = bigTen;
			if (radix != 16)
			    numStrValid = false;
			break;
	    case 'b':
	    case 'B':	digit = bigEleven;
			if (radix != 16)
			    numStrValid = false;
			break;
	    case 'c':
	    case 'C':	digit = bigTwelve;
			if (radix != 16)
			    numStrValid = false;
			break;
	    case 'd':
	    case 'D':	digit = bigThirteen;
			if (radix != 16)
			    numStrValid = false;
			break;
	    case 'e':
	    case 'E':	digit = bigFourteen;
			if (radix != 16)
			    numStrValid = false;
			break;
	    case 'f':
	    case 'F':	digit = bigFifteen;
			if (radix != 16)
			    numStrValid = false;
			break;
	    default:
			numStrValid = false;
			break;

	    }

	    if (! numStrValid)
		break;	// out of loop

	    // accumulate numeric value
	    *this *= bigRadix;
	    *this += digit;
	}

	if (isZero(*this))
	    this->sign = Positive;	// keep zero positive
	else
	    this->sign = signArg;
    }

    /*********************************
     * for stream input, such as cin *
     *********************************/
    void readFrom(std::istream& strm)
    {
	int radix;
	std::string numStr;
	unsigned start;
	Sign signArg;

	strm >> numStr;

	if (strm.flags() & std::ios::hex)
	    radix = 16;
	else if (strm.flags() & std::ios::oct)
	    radix = 8;
	else	// assume decimal
	    radix = 10;

	if (numStr[0] == '-') {
	    signArg = Negative;
	    start = 1;
	} else if (numStr[0] == '+') {
	    signArg = Positive;
	    start = 1;
	} else {
	    signArg = Positive;
	    start = 0;
	}

	strToBigInteger(numStr, radix, start, signArg);
    }

    /***********************************
     * for stream output, such as cout *
     ***********************************/
    void printOn(std::ostream& strm) const
    {
	BigInteger bigRadix;
	std::stack<int> digitStack;

	if (this->sign == Negative)
	    strm << "-";

	if (isZero(*this)) {
	    strm << "0BI";
	    return;
	}

	if (strm.flags() & std::ios::hex)
	    bigRadix = 16;
	else if (strm.flags() & std::ios::oct)
	    bigRadix = 8;
	else	// assume decimal
	    bigRadix = 10;

	BigInteger tmp(*this);
	BigInteger remainder;

	// extract individual digits from number (in reverse order)
	while (!isZero(tmp)) {
	    tmp = binaryDivide(tmp, bigRadix, remainder);
	    digitStack.push(remainder.magnitude[0]);
	}

	// display individual digits in the proper order
	while (!digitStack.empty()) {
	    strm << digitStack.top();
	    digitStack.pop();
	}

	strm << "BI";	// denote a big integer

	// for debugging, show number of CHUNKs needed to store value
	if (this->magnitude.size() > 1)
	    strm << this->magnitude.size();
    }

    /************************************************************
     * utility routine for comparing two BigInteger magnitudes, *
     * ignoring sign                                            *
     ************************************************************/
    NumericConstant
    compareMagnitude(const BigInteger &arg1, const BigInteger &arg2)
    const
    {
	ULONG index;
	ULONG arg1Size = arg1.magnitude.size();
	ULONG arg2Size = arg2.magnitude.size();

	if (arg1Size > arg2Size)
		return Arg1Larger;
	else if (arg1Size < arg2Size)
	    return Arg1Smaller;

	// both arguments are the same size
	// compare from most significant to least significant 'digit'

	for (index = arg1Size; index > 0; --index) {
	    ULONG i = index - 1;
	    if (arg1.magnitude[i] < arg2.magnitude[i])
		return Arg1Smaller;
	    else if (arg1.magnitude[i] > arg2.magnitude[i])
		return Arg1Larger;
	}

	// if we get here, arguments are equal
	return ArgsEqual;
    }

    /*******************************************************
     * utility routine for comparing two BigInteger values *
     *******************************************************/
    NumericConstant
    compareSignAndMagnitude(const BigInteger &arg1, const BigInteger &arg2)
    const
    {
	ULONG index;
	ULONG arg1Size = arg1.magnitude.size();
	ULONG arg2Size = arg2.magnitude.size();

	if (arg1.sign == Positive && arg2.sign == Negative)
	    return Arg1Larger;
	else if (arg1.sign == Negative && arg2.sign == Positive)
	    return Arg1Smaller;

	// both arguments are the same sign

	NumericConstant result = compareMagnitude(arg1, arg2);
	if (result == Arg1Larger && arg1.sign == Negative)
	    result = Arg1Smaller;
	else if (result == Arg1Smaller && arg1.sign == Negative)
	    result = Arg1Larger;

	return result;
    }

    /**************************************************************
     * utility routine for &= and |= operators                    *
     * 'other' may be modified, do not pass it by reference       *
     * "this" is set to the longer of "this" and other            *
     * so it may be modified also; this function can not be const *
     **************************************************************/
    void andOrEquals(NumericConstant operType, BigInteger other)
    {
	ULONG otherSize = other.magnitude.size();
	ULONG mySize = this->magnitude.size();
	ULONG compareSize;
	ULONG i;

	// make sure both magnitudes are the same size
	if (mySize > otherSize) {
	    other.magnitude.resize(mySize, 0);
	    compareSize = mySize;
	} else {	// otherSize >= mySize
	    if (otherSize > mySize)
		this->magnitude.resize(otherSize, 0);
	    compareSize = otherSize;
	}

	if (operType == OrEquals) {
	    for (i = 0; i < compareSize; ++i)
		this->magnitude[i] |= other.magnitude[i];
	} else {	// operType == AndEquals
	    for (i = 0; i < compareSize; ++i)
		this->magnitude[i] &= other.magnitude[i];
	}

	popLeadingZeros();
    }

    /****************************************
     * shift BigInteger left 1 bit position *
     ****************************************/
    void shiftMeLeft1()
    {
	ULONG i;
	ULONG mySize = this->magnitude.size();
	CHUNK newLowBit, nextLowBit = 0;

	// shift from low order to high order
	for (i = 0; i < mySize; ++i) {
	    newLowBit = nextLowBit;
	    if (this->magnitude[i] & HIGH_BIT)
		nextLowBit = LOW_BIT;
	    else
		nextLowBit = 0;
	    this->magnitude[i] <<= 1;
	    this->magnitude[i] |= newLowBit;
	}

	// Unlike standard C++ left shifts,
	// it is impossible to shift 1 out of the high bit position.
	// We just make the number bigger.
	if (nextLowBit)
	    this->magnitude.push_back(nextLowBit);
    }

    /**************************************************************************
     * shift BigInteger right 1 bit position                                  *
     * this is an unsigned shift; zeroes are moved into vacated bit positions *
     **************************************************************************/
    void shiftMeRight1()
    {
	ULONG index;
	ULONG mySize = this->magnitude.size();
	CHUNK newHighBit, nextHighBit = 0;

	// shift from high order to loworder order
	for (index = mySize; index > 0; --index) {
	    ULONG i = index - 1;
	    newHighBit = nextHighBit;
	    if (this->magnitude[i] & LOW_BIT)
		nextHighBit = HIGH_BIT;
	    else
		nextHighBit = 0;
	    this->magnitude[i] >>= 1;
	    this->magnitude[i] |= newHighBit;
	}

	// if high order magnitude multi-chunk is now zero, get rid of it
	if (mySize > 1 && this->magnitude.back() == 0)
	    this->magnitude.pop_back();
    }

    /**************************************************************
     * utility routine for all addition and subtraction operators *
     **************************************************************/
    BigInteger plusEquals (BigInteger other)
    {
	if (isZero(*this)) {
	    *this = other;
	    return *this;
	}

	if (isZero(other))
	    return *this;

	// if we reach here, neither operand is zero
	if (this->sign == other.sign) {
	    this->addMagnitude(other);
	    return *this;
	}

	// if we reach here, signs of the operands differ

	// if magnitudes are equal, return zero
	if (this->magnitude == other.magnitude) {
	    makeZero();
	    return *this;
	}

	BigInteger *large, *small;

	// arrange arguments in proper order for subtractMagnitude
	if (compareMagnitude(*this, other) == Arg1Larger) {
	    large = this;
	    small = &other;
	} else {
	    small = this;
	    large = &other;
	}

	BigInteger tmp(*large);
	tmp.subtractMagnitude(*small);

	*this = tmp;	// sign of larger is sign of result
	return *this;
    }

    public:
    /****************
     * Constructors *
     ****************/
    BigInteger() :sign(Positive)
    {
	magnitude.push_back(0);
    }

    BigInteger(const int val)
    {
	if (val >= 0) {
	    sign = Positive;
	    magnitude.push_back(val);
	} else {
	    sign = Negative;
	    magnitude.push_back(-val);
	}
    }

    BigInteger(const unsigned int val) :sign(Positive)
    {
	magnitude.push_back(val);
    }

    BigInteger(const long val)
    {
	if (val >= 0) {
	    sign = Positive;
	    magnitude.push_back(val);
	} else {
	    sign = Negative;
	    magnitude.push_back(-val);
	}
    }

    BigInteger(const unsigned long val) :sign(Positive)
    {
	magnitude.push_back(val);
    }

    BigInteger(const long long val)
    {
	if (val >= 0) {
	    sign = Positive;
	    magnitude.push_back(val);
	} else {
	    sign = Negative;
	    magnitude.push_back(-val);
	}
    }

    BigInteger(const unsigned long long val) :sign(Positive)
    {
	magnitude.push_back(val);
    }

    /********************************************************************
     * allow a string to initialize BigIntegers                         *
     * otherwise there would be no way in initialize them to big values *
     ********************************************************************/
    BigInteger(const std::string numStr)
    {
	int radix;
	unsigned start = 0;
	Sign signArg;

	if (numStr.substr(0,1) == "-") {
	    signArg = Negative;
	    start = 1;
	} else if (numStr.substr(0,1) == "+") {
	    signArg = Positive;
	    start = 1;
	} else {
	    signArg = Positive;
	    start = 0;
	}

	if (numStr.substr(start,2) == "0x") {
	    radix = 16;
	    start += 2;
	} else if (numStr.substr(start,2) == "0X") {
	    radix = 16;
	    start += 2;
	} else if (numStr.substr(start,1) == "0") {
	    radix = 8;
	    start += 1;
	} else
	    radix = 10;

	strToBigInteger(numStr, radix, start, signArg);
    }

    /*************
     * Operators *
     *************/
    BigInteger operator= (const BigInteger &other)
    {
	if (this != &other) {
	    this->sign = other.sign;
	    this->magnitude = other.magnitude;
	}

	return *this;
    }

    bool operator== (const BigInteger &other) const
    {
	return this->magnitude == other.magnitude
		&& this->sign == other.sign;
    }

    bool operator!= (const BigInteger &other) const
    {
	return this->magnitude != other.magnitude
		|| this->sign != other.sign;
    }

    bool operator> (const BigInteger &other) const
    {
	return compareSignAndMagnitude(*this, other) == Arg1Larger;
    }

    bool operator<= (const BigInteger &other) const
    {
	return compareSignAndMagnitude(*this, other) != Arg1Larger;
    }

    bool operator< (const BigInteger &other) const
    {
	return compareSignAndMagnitude(*this, other) == Arg1Smaller;
    }

    bool operator>= (const BigInteger &other) const
    {
	return compareSignAndMagnitude(*this, other) != Arg1Smaller;
    }

    BigInteger operator++ () 	// pre-increment
    {
	static const BigInteger bigOne(1);

	return plusEquals(bigOne);
    }

    BigInteger operator++ (int)	// post-increment
    {
	static const BigInteger bigOne(1);

	BigInteger retVal = *this;
	plusEquals(bigOne);
	return retVal;
    }

    BigInteger operator-- () 	// pre-decrement
    {
	static const BigInteger minusOne(-1);

	return plusEquals(minusOne);
    }

    BigInteger operator-- (int)	// post-decrement
    {
	static const BigInteger minusOne(-1);

	BigInteger retVal = *this;
	plusEquals(minusOne);
	return retVal;
    }

    BigInteger operator<<= (BigInteger shiftAmount)
    {
	while (shiftAmount-- > 0)
	    shiftMeLeft1();

	return *this;
    }

    BigInteger operator<< (BigInteger shiftAmount) const
    {
	BigInteger answer(*this);

	answer <<= shiftAmount;
	return answer;
    }

    BigInteger operator>>= (BigInteger shiftAmount)
    {
	while (shiftAmount-- > 0)
	    shiftMeRight1();

	return *this;
    }

    BigInteger operator>> (const BigInteger shiftAmount) const
    {
	BigInteger answer(*this);

	answer >>= shiftAmount;
	return answer;
    }

    BigInteger operator|= (BigInteger other)
    {
	andOrEquals(OrEquals, other);
	return *this;
    }

    BigInteger operator| (const BigInteger other) const
    {
	BigInteger answer(*this);

	answer |= other;
	return answer;
    }

    BigInteger operator&= (BigInteger other)
    {
	andOrEquals(AndEquals, other);
	return *this;
    }

    BigInteger operator& (const BigInteger other) const
    {
	BigInteger answer(*this);

	answer &= other;
	return answer;
    }

    bool operator&& (const BigInteger other) const
    {
	return !isZero(*this) && !isZero(other);
    }

    bool operator|| (const BigInteger other) const
    {
	return !isZero(*this) || !isZero(other);
    }

    BigInteger operator+= (BigInteger other)
    {
	return plusEquals(other);
    }

    BigInteger operator-= (BigInteger other)
    {
	BigInteger tmp(-other);
	return plusEquals(tmp);
    }

    BigInteger operator+ (const BigInteger other) const
    {
	BigInteger answer(*this);

	answer.plusEquals(other);
	return answer;
    }

    BigInteger operator- (const BigInteger other) const
    {
	BigInteger answer(*this);
	BigInteger tmp(-other);

	answer.plusEquals(tmp);
	return answer;
    }

    // Unary minus operator
    BigInteger operator- () const
    {
	BigInteger answer(*this);
	flipSign(answer);
	return answer;
    }

    // Unary plus operator
    BigInteger operator+ () const
    {
	BigInteger answer(*this);
	return answer;
    }

    BigInteger operator* (BigInteger other) const
    {
	return binaryMultiply(*this, other);
    }

    BigInteger operator*= (BigInteger other)
    {
	*this = binaryMultiply(*this, other);
	return *this;
    }

    BigInteger operator/ (BigInteger other) const
    {
	BigInteger remainder;

	return binaryDivide(*this, other, remainder);
    }

    BigInteger operator/= (BigInteger other)
    {
	BigInteger remainder;

	*this = binaryDivide(*this, other, remainder);
	return *this;
    }

    BigInteger operator% (BigInteger other) const
    {
	BigInteger remainder;

	binaryDivide(*this, other, remainder);
	return remainder;
    }

    BigInteger operator%= (BigInteger other)
    {
	BigInteger remainder;

	binaryDivide(*this, other, remainder);
	*this = remainder;
	return *this;
    }
};

// stream input and output
inline std::ostream& operator<< (std::ostream &strm, BigInteger bigI)
{
    bigI.printOn(strm);
    return strm;
}


inline std::istream& operator>> (std::istream &strm, BigInteger &bigI)
{
    bigI.readFrom(strm);
    return strm;
}

/* Driver program to for testing */
int main(int argc, char *argv[])
{
    static const BigInteger bigZero(0);
    static const BigInteger bigTen(10);
    if (argc > 1) {
	static const std::string Octal("octal");
	static const std::string Hex("hex");
	if (Octal == argv[1]) {
	    std::cin >> std::oct;
	    std::cout << std::oct;
	} else if (Hex == argv[1]) {
	    std::cin >> std::hex;
	    std::cout << std::hex;
	}
    }
    std::cout << "Input and Output in ";
    if (std::cin.flags() & std::ios::hex)
	std::cout << "hex\n";
    else if (std::cin.flags() & std::ios::oct)
	std::cout << "octal\n";
    else
	std::cout << "decimal\n";
    try {
	BigInteger i(static_cast<CHUNK>(-3));
	BigInteger j(8000000000000000011LL);
	BigInteger k(i);
	DB(i==k);
	DB4(i, j, k, i < j);
	i += j;
	DB3(i, j, k);
	DB4(i>j, j>k, i<j, j<k);
	DB3(i==k,i==j, j==k);
	BigInteger a1(-7);
	BigInteger a2(100);
	BigInteger a3;
	DB3(a1, a2, a3);
	a2 += a1;
	a3 = a1 + a2;
	DB3(a1, a2, a3);
	DB3(a1, a2-a1, a3);
	DB3(a1, a2+a1, a3);
	DB(a2-=a3);
	DB(a2);
	i = j + j;
	DB(i);
	DB(i+k);
	i <<= 32;
	DB(i);
	i >>= 4;
	DB(i);
	DB(a1+a2+a3);
	DB(a2);
	DB(a2++);
	DB(a2);
	DB(++a2);
	DB(a2);
	DB(a2--);
	DB(a2);
	DB(--a2);
	DB(a2);

	i &= 1;
	BigInteger foo(298653);
	DB(foo);
	DB(foo/=17);
	DB(foo);
	DB(foo/16);
	DB(foo*16);
	DB(foo*=256);
	DB(foo);
	BigInteger foo2(-1);
	DB(foo2);
	DB(foo*= foo2);
	DB(foo);
	DB(foo % 10);
	DB(foo %= 10);
	DB3(bigZero&&bigZero, bigZero&&bigTen, bigTen&&bigTen);
	DB3(bigZero||bigZero, bigZero||bigTen, bigTen||bigTen);
	DB(foo);
	std::cout << "Enter new value for foo: ";
	std::cin >> foo;
	DB(foo);
	std::cout << "Enter new value for foo2: ";
	std::cin >> foo2;
	DB(foo2);
	DB(foo * foo2);
	DB(foo / foo2);
	DB(foo % foo2);
	BigInteger strToNum1("123456789012345678901234567890");
	BigInteger strToNum2("+987654321098765432109876543210");
	BigInteger strToNum3("-987654321098765432109876543210");
	DB(strToNum1);
	DB(strToNum2);
	DB(strToNum3);
	BigInteger strToNumHex("0xaaaaaaaaaaaaaaaaaaaaabbbbbbbbbbbbbbbbb");
	BigInteger strToNumOctal("012345671234567123456712345671234567001");
	DB(strToNumHex);
	DB(strToNumOctal);
    } catch (char const* &e) {
	std::cout << "Error: " << e << std::endl;
	return 1;
    }
}
