#pragma once

#include <utility>
#include <string.h>
#include <assert.h>
#include <optional>

namespace format
{
	template<typename OutputIt>
	struct format_output_it {
		format_output_it(OutputIt& it): it{it} {}

		void write(const char* str){
			while(*str)
				it.putc(*str++);
		}

		void write(const char c){
			it.putc(c);
		}

		private:
		OutputIt& it;
	};

	struct format_args {
		std::optional<char> fill, align, sign;
		std::optional<bool> alternate;
		std::optional<size_t> width, precision;
		std::optional<char> type;
	};

	namespace internal {
		
		template<typename OutputIt, typename T>
		void format_integer(format_output_it<OutputIt>& out, [[maybe_unused]] format_args args, T v){
			if(!args.type)
				args.type = 'd';
			
			if(!args.alternate)
				args.alternate = false;

			size_t base = 0;
			bool capital = false;
			const char* base_prefix = "";
			
			switch (*args.type)
			{
			// Type options
			case 'b': // Binary
				base = 2;
				base_prefix = "0b";
				break;
			case 'B':
				base = 2;
				base_prefix = "0B";
				break;
			case 'd': // Decimal
				base = 10;
				base_prefix = "";
				break;
			case 'o': // Octal
				base = 8;
				base_prefix = "0";
				break;
			case 'x': // Hex, lowercase
				base = 16;
				base_prefix = "0x";
				break;
			case 'X': // Hex, uppercase
				base = 16;
				base_prefix = "0X";
				capital = true;
				break;            
			
			default:
				assert(!"Unknown type specifier");
				break;
			}


			if(*args.alternate)
				out.write(base_prefix);

			auto format_number = [](uint64_t value, char* str, int base, bool capital) -> char* {
				if(base < 2 || base > 36){
					*str = '\0';
					return str;
				}

				char* rc = str, *ptr = str;

				if((int64_t)value < 0 && base == 10){
					value = (uint64_t)(-((int64_t)value));
					*ptr++ = '-';
				}

				char* low = ptr;

				do {
					const char* lower_case = "zyxwvutsrqponmlkjihgfedcba9876543210123456789abcdefghijklmnopqrstuvwxyz";
					const char* upper_case = "ZYXWVUTSRQPONMLKJIHGFEDCBA9876543210123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
					const char* digits = capital ? upper_case : lower_case;

					*ptr++ = digits[35 + value % base];
					value /= base;
				} while (value);

				*ptr-- = '\0';

				while(low < ptr){
					char tmp = *low;
					*low++ = *ptr;
					*ptr-- = tmp;
				}
				return rc;
			};

			char int_buf[100]{};
			format_number(v, int_buf, base, capital);
			out.write(int_buf);
		};
	}

	template<typename T>
	struct formatter {
		template<typename OutputIt>
		static void format(format_output_it<OutputIt>& it, [[maybe_unused]] format_args args, T item);
	};

	template<>
	struct formatter<const char*> {
		template<typename OutputIt>
		static void format(format_output_it<OutputIt>& it, [[maybe_unused]] format_args args, const char* item){
			it.write(item);
		}
	};

	template<>
	struct formatter<char*> {
		template<typename OutputIt>
		static void format(format_output_it<OutputIt>& it, [[maybe_unused]] format_args args, char* item){
			formatter<const char*>::format(it, args, item);
		}
	};

	template<>
	struct formatter<std::string> {
		template<typename OutputIt>
		static void format(format_output_it<OutputIt>& it, [[maybe_unused]] format_args args, std::string item){
			it.write(item.c_str());
		}
	};

	#define INT_IMPL(T) \
		template<> \
		struct formatter<T> { \
			template<typename OutputIt> \
			static void format(format_output_it<OutputIt>& it, [[maybe_unused]] format_args args, T item){ \
				internal::format_integer(it, args, item); \
			} \
		};

	INT_IMPL(short int)
	INT_IMPL(unsigned short int)

	INT_IMPL(int)
	INT_IMPL(unsigned int)

	INT_IMPL(long int)
	INT_IMPL(unsigned long int)

	INT_IMPL(long long int)
	INT_IMPL(unsigned long long int)

	#undef INT_IMPL

	template<>
	struct formatter<char> {
		template<typename OutputIt>
		static void format(format_output_it<OutputIt>& it, [[maybe_unused]] format_args args, char item){
			if(!args.type)
				args.type = 'c';

			if(args.type == 'c')
				it.write(item);
			else
				formatter<uint64_t>::format(it, args, item);
		}
	};

	template<>
	struct formatter<bool> {
		template<typename OutputIt>
		static void format(format_output_it<OutputIt>& it, [[maybe_unused]] format_args args, bool item){
			if(!args.type)
				args.type = 's'; // Textual is default

			switch (*args.type)
			{
			case 'd':
				it.write(item ? "1" : "0");
				break;
			case 's':
				it.write(item ? "true" : "false");
				break;
			}
		}
	};

	template<>
	struct formatter<void*> {
		template<typename OutputIt>
		static void format(format_output_it<OutputIt>& it, [[maybe_unused]] format_args args, void* item){
			formatter<uintptr_t>::format(it, {.alternate = true, .type = 'x'}, (uintptr_t)item); // Default is 0xYYYYYYYYYYYYYYYY where Y is the pointer
		}
	};

	template<>
	struct formatter<std::nullptr_t> {
		template<typename OutputIt>
		static void format(format_output_it<OutputIt>& it, [[maybe_unused]] format_args args, std::nullptr_t item){
			formatter<uint64_t>::format(it, {.alternate = true, .type = 'x'}, 0);
		}
	};

	namespace internal {
		template<typename OutputIt, typename... Args>
		void format_int(format_output_it<OutputIt> out, const char* fmt, Args&&... args);

		template<typename OutputIt, typename T, typename... Args>
		void format_part(format_output_it<OutputIt>& out, [[maybe_unused]] format_args f_args, const char* fmt, T v, Args&&... args){
			formatter<T>::format(out, f_args, v);
			format_int(out, fmt, std::forward<Args>(args)...);
		}

		template<typename OutputIt>
		void format_part(format_output_it<OutputIt>& out, [[maybe_unused]] format_args args, const char* fmt){
			// No parts left, print whatever is left of fmt, and start return chain
			out.write(fmt);
			return;
		} 

		template<typename OutputIt, typename... Args>
		void format_int(format_output_it<OutputIt> out, const char* fmt, Args&&... args){
			while(*fmt){
				if(strncmp(fmt, "{{", 2) == 0){
					out.write("{");
					fmt += 2;
				} else if(strncmp(fmt, "}}", 2) == 0) {
					out.write("}");
					fmt += 2;
				} else if(*fmt == '{'){
					// General formatting

					// TODO: Parse args
					fmt++;

					assert(*fmt == ':' || *fmt == '}' || !"arg-id is unsupported");

					format_args options{};
					while(*fmt != '}'){
						if(*fmt == 'A' || *fmt == 'a' || *fmt == 'b' || *fmt == 'B' || *fmt == 'c' || *fmt == 'd' || *fmt == 'e' || *fmt == 'E' || \
						   *fmt == 'f' || *fmt == 'F' || *fmt == 'g' || *fmt == 'G' || *fmt == 'o' || *fmt == 'p' || *fmt == 's' || *fmt == 'x' || \
						   *fmt == 'X') // Type
							options.type = *fmt;
						else if(*fmt == '<' || *fmt == '>' || *fmt == '^') // Align
							options.type = *fmt;
						else if(*fmt == '+' || *fmt == '-' || *fmt == ' ') // Sign
							options.sign = *fmt;
						else if(*fmt == '#')
							options.alternate = true;
						else if(*fmt == ':')
							;
						else
							assert(!"Unknown character in options");

						fmt++;
					}

					fmt++; // Skip final '}'

					format_part(out, options, fmt, std::forward<Args>(args)...);
					return;
				} else {
					out.write(*fmt);
					fmt++;
				}
			}
		}
	} // namespace internal

	template<typename OutputIt, typename... Args>
	void format_to(OutputIt& out, const char* fmt, Args&&... args){
		internal::format_int(format_output_it{out}, fmt, std::forward<Args>(args)...);
	}
} // namespace format

template<typename... Args>
void print(const char* fmt, Args&&... args){
    struct {
        void putc(const char c){
            ::putchar(c);
        }
    } it;

    format::format_to(it, fmt, std::forward<Args>(args)...);
}

template<typename... Args>
std::string format_to_str(const char* fmt, Args&&... args){
    struct {
        void putc(const char c){
			//::putchar(c);
			str.push_back(c);
        }
		std::string str{};
    } it{};

    format::format_to(it, fmt, std::forward<Args>(args)...);
	return it.str;
}