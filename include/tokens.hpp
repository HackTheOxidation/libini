#ifndef TOKENS_HPP_
#define TOKENS_HPP_

#include <algorithm>
#include <concepts>
#include <cstddef>
#include <variant>
#include <string>
#include <type_traits>

namespace libini {

  // Strongly-typed Enum for representing token types of interest.
  enum class TokenType {
    LBrace,
    RBrace,
    Equals,
    DoubleQuote,
    SingleQuote,
    Section,
    Identifier,
    String,
    Number,
    Null,
    EndOfFile,
  };

  // Structure for representing numerical values.
  struct IniNumber {
    constexpr IniNumber(const float f) noexcept
      : token_value(f) {}
    constexpr IniNumber(const IniNumber& other) noexcept
      : token_value(other.token_value) {}
    constexpr IniNumber& operator=(const IniNumber& other) noexcept {
      if (this != &other)
	token_value = other.token_value;
      return *this;
    }
    static constexpr TokenType token_type = TokenType::Number;
    float token_value;
    using value_type = float;
  };

  // Structure for representing string values.
  struct IniString {
    IniString(const std::string str) noexcept
      : token_value(str) {}
    IniString(const IniString& other) noexcept
      : token_value(other.token_value) {}
    IniString& operator=(const IniString& other) noexcept {
      if (this != &other)
	token_value = other.token_value;
      return *this;
    }
    static constexpr TokenType token_type = TokenType::String;
    std::string token_value;
    using value_type = std::string;
  };

  // Structure for representing identifiers (variable names).
  struct IniIdentifier {
    IniIdentifier(const std::string str) noexcept
      : token_value(str) {}
    IniIdentifier(const IniIdentifier& other) noexcept
      : token_value(other.token_value) {};
    IniIdentifier& operator=(const IniIdentifier& other) noexcept {
      if (this != &other) 
	token_value = other.token_value;
      return *this;
    }
    static constexpr TokenType token_type = TokenType::Identifier;
    std::string token_value;
    using value_type = std::string;
  };

  // Structure for representing sections ([<name>]).
  struct IniSection {
    IniSection(const std::string str) noexcept
      : token_value(str) {}
    IniSection(const IniSection& other) noexcept
      : token_value(other.token_value) {}
    IniSection& operator=(const IniSection& other) noexcept {
      if (this != &other)
	token_value = other.token_value;
      return *this;
    }
    static constexpr TokenType token_type = TokenType::Section;
    std::string token_value;
    using value_type = std::string;
  };

  // Structure for representing null value. Valueless.
  struct IniNull {
    constexpr IniNull() noexcept = default;
    constexpr IniNull(const IniNull&) noexcept = default;
    static constexpr TokenType token_type = TokenType::Null;
  };

  struct IniLBrace {
    constexpr IniLBrace() noexcept = default;
    constexpr IniLBrace(const IniLBrace&) noexcept = default;
    static constexpr TokenType token_type = TokenType::LBrace;
  };

  struct IniRBrace {
    constexpr IniRBrace() noexcept = default;
    constexpr IniRBrace(const IniRBrace&) noexcept = default;
    static constexpr TokenType token_type = TokenType::RBrace;
  };

  struct IniEquals {
    constexpr IniEquals() noexcept = default;
    constexpr IniEquals(const IniEquals&) noexcept = default;
    static constexpr TokenType token_type = TokenType::Equals;
  };

  struct IniDoubleQuote {
    constexpr IniDoubleQuote() noexcept = default;
    constexpr IniDoubleQuote(const IniDoubleQuote&) noexcept = default;
    static constexpr TokenType token_type = TokenType::DoubleQuote;
  };

  struct IniSingleQuote {
    constexpr IniSingleQuote() noexcept = default;
    constexpr IniSingleQuote(const IniSingleQuote&) noexcept = default;
    static constexpr TokenType token_type = TokenType::SingleQuote;
  };

  // Using std::variant to create a type-alias for all .ini tokens.
  using IniVariant = std::variant<IniSection, IniNumber, IniString,
				  IniIdentifier, IniNull, IniLBrace,
				  IniRBrace, IniEquals, IniDoubleQuote,
				  IniSingleQuote>;

  template<typename T>
  concept TokenTyped = requires(T a) {
    { T::token_type } -> std::convertible_to<TokenType>;
  };

  template<typename T>
  concept TokenValued = requires(T a) {
     T::token_value;
  };

  template<typename T>
  concept ParsableToken = TokenTyped<T> && TokenValued<T>;

}; // namespace libini
#endif
