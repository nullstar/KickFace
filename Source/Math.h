#pragma once


namespace Math
{
	inline int positiveModulo(int val, int mod)
	{
		return (val % mod + mod) % mod;
	}
}