#include "Saffron/SaffronPCH.h"
#include "Saffron/System/Time.h"

namespace Se
{

Time::Time(float seconds)
	: m_Time(seconds)
{
}

Time &Time::operator+(const Time &rhs)
{
	m_Time += rhs.m_Time;
	return *this;
}

Time &Time::operator+=(const Time &rhs)
{
	return *this = *this + rhs;
}

Time &Time::operator+=(float seconds)
{
	m_Time += seconds;
	return *this;
}

float Time::operator()() const
{
	return m_Time;
}

float Time::sec() const
{
	return m_Time;
}

float Time::ms() const
{
	return m_Time * 1000.0f;
}

}