#ifndef LEXER_HPP_
#define LEXER_HPP_

#include <algorithm>
#include <concepts>
#include <deque>
#include <exception>
#include <functional>
#include <fstream>
#include <iterator>
#include <memory_resource>
#include <stdexcept>
#include <type_traits>

#include "tokens.hpp"

namespace libini {

  using IniTokens = std::pmr::deque<IniVariant>;
  using IniIterator = std::istream_iterator<char>;
  using Predicate = std::function<bool(char)>;
  using ComposerOp = std::function<bool(bool, bool)>;

  template<typename T>
  concept IniTokenizer = requires(T a, std::pmr::polymorphic_allocator<IniVariant> allocator) {
    { a.tokenize(allocator) } -> std::same_as<IniTokens>;
    { a(allocator) } -> std::same_as<IniTokens>;
  };

  static const Predicate compose(Predicate p, Predicate q, ComposerOp op = std::logical_or<bool>()) {
    // Composes two predicate functions into one.
    return [p, q, op](char c) { return op(p(c), q(c)); };
  }

  template<typename T>
  requires std::same_as<T, char>
  static constexpr Predicate make_predicate(T d) {
    return [d](T c) { return c == d; };
  }

  template<typename T, typename... Args>
  static constexpr Predicate make_predicate(T d, Args... ds) {
    if constexpr(sizeof...(ds) > 0) {
      return [d, ds...](T c) {
	return make_predicate(d)(c) || make_predicate(ds...)(c);
      };
    } else
      return make_predicate(d);
  }

  static const auto is_whitespace = make_predicate(' ', '\t');

  static const auto is_comment = make_predicate('#');

  static const auto is_eol = make_predicate('\n', '\r');

  static constexpr bool is_numeric(char c) {
    // Checks if c is between 0 and 9.
    return '0' <= c && c <= '9';
  };

  static const auto is_whitespace_or_eol = compose(is_eol, is_whitespace);

  class IniLexer {
  public:
    IniLexer(const std::string file_name) noexcept;

    ~IniLexer() noexcept;

    // A lexer should not be Copyable as it relies on ifstream,
    // which is not Copyable.
    IniLexer(const IniLexer& other) = delete;
    IniLexer& operator =(const IniLexer&) = delete;

    // But it should be movable.
    IniLexer(IniLexer&& other) noexcept;

    IniLexer& operator=(IniLexer&& other) noexcept;

    // Dispatch to tokenize().
    IniTokens operator ()(std::pmr::polymorphic_allocator<IniVariant> allocator);

    // Tokenizes (reads and converts content to token representations) the .ini file pointed to by the fstream. 
    IniTokens tokenize(std::pmr::polymorphic_allocator<IniVariant> allocator); 

  private:
    std::ifstream stream_;
    const std::string file_name_; 

    /*
     * Read and tokenize an entire .ini file.
     */
    void read_all(IniTokens& tokens) noexcept;
    /*
     * Peeks in the file stream and determines what the next target is.
     */
    TokenType next_token(IniIterator& fiter,
			 IniIterator& eos,
			 const TokenType previous,
			 Predicate& delimiter) noexcept;
    /*
     * Skips a single-line comment.
     */
    void skip_comment(IniIterator& fiter, IniIterator& eos) noexcept;
    /*
     * Reads a 'name' given a predicate to determine whether or not to stop.
     * This is used for sections, identifiers, strings, numbers etc.
     */
    std::string read_name(IniIterator& fiter,
			  IniIterator& eos,
			  Predicate delimiter,
			  Predicate is_ignorable = [](char) { return false; }) noexcept;

    std::string read_number(IniIterator& fiter,
		    	    IniIterator& eos) noexcept;
  };
};

#endif
