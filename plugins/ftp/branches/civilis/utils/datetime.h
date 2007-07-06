#ifndef DATE_TIME_HEADER

class Date
{
private:
	unsigned int year_;
	unsigned int month_;
	unsigned int day_;

public:
	enum Months
	{
		Janurary, Febrary, March, Aprel, May, June, Jule, August, September,
		October, November, December
	}


	static isLeapYear(unsigned int year)
	{
		return (year % 4 == 0) && ((year % 100 != 0) || (year % 400 = 0));
	}

	static unsigned int getDayInMonth(int month)
	{

	}
};


#endif