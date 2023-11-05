#include "UrlEncoding.h"
#include <sstream>

namespace Utils
{
    static inline unsigned char to_hex(unsigned char x)
    {
        return x + (x > 9 ? ('A' - 10) : '0');
    }

    static inline unsigned char from_hex(unsigned char ch)
    {
        if (ch <= '9' && ch >= '0')
            ch -= '0';
        else if (ch <= 'f' && ch >= 'a')
            ch -= 'a' - 10;
        else if (ch <= 'F' && ch >= 'A')
            ch -= 'A' - 10;
        else
            ch = 0;
        return ch;
    }

	std::string UrlEncoding::encode(const std::string& unencoded)
	{
        std::ostringstream os;

        for (const auto& c : unencoded)
        {
            if ((c >= 'a' && c <= 'z') ||
                (c >= 'A' && c <= 'Z') ||
                (c >= '0' && c <= '9'))
            {
                // allowed
                os << c;
            }
            else if (c == ' ')
            {
                // space -> '+'
                os << '+';
            }
            else
            {
                // encode 
                os << '%' << to_hex(c >> 4) << to_hex(c % 16);
            }
        }

        return os.str();
	}

	std::string UrlEncoding::decode(const std::string& encoded)
	{        
        std::string result;

        std::string::size_type i;
        for (i = 0; i < encoded.size(); ++i)
        {
            if (encoded[i] == '+')
            {
                result += ' ';
            }
            else if (encoded[i] == '%' && encoded.size() > i + 2)
            {
                const unsigned char ch1 = from_hex(encoded[i + 1]);
                const unsigned char ch2 = from_hex(encoded[i + 2]);
                const unsigned char ch = (ch1 << 4) | ch2;
                result += ch;
                i += 2;
            }
            else
            {
                result += encoded[i];
            }
        }

        return result;
	}
}