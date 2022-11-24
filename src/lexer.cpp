#include <lexer.hpp>

namespace libini {

  IniLexer::IniLexer(const std::string file_name) noexcept
    : stream_{}, file_name_(file_name) {
  }

  IniLexer::~IniLexer() noexcept {
    if (stream_.is_open())
      stream_.close();
  }

  IniLexer::IniLexer(IniLexer&& other) noexcept
    : stream_{std::move(other.stream_)}, file_name_{std::move(other.file_name_)} {
  }

  IniLexer& IniLexer::operator=(IniLexer&& other) noexcept {
    if (this != &other) {
      if (stream_.is_open())
	stream_.close();
      std::swap(other.stream_, stream_);
    }

    return *this;
  };

  // Dispatch to tokenize().
  IniTokens IniLexer::operator ()(std::pmr::polymorphic_allocator<IniVariant> allocator) {
    return tokenize(allocator);
  }

  // Tokenizes (reads and converts content to token representations) the .ini file pointed to by the fstream. 
  IniTokens IniLexer::tokenize(std::pmr::polymorphic_allocator<IniVariant> allocator) {
    IniTokens tokens{allocator};
    read_all(tokens);
    return tokens;
  }

  /*
   * Read and tokenize an entire .ini file.
   */
  void IniLexer::read_all(IniTokens& tokens) noexcept {
    if (!stream_.is_open())
      stream_.open(file_name_, std::fstream::ios_base::in);

    // Tell the ifstream to not skip whitespace, EOL, etc.
    stream_ >> std::noskipws;

    IniIterator eos;
    IniIterator fiter(stream_);
    auto none = TokenType::Null;
    auto eof = TokenType::EndOfFile;
    Predicate delimiter = [](char){return false;};

    for(auto token = none; token != eof; token = next_token(fiter, eos, token, delimiter)) {
      // TODO: The rest of the lexer logic...
      switch (token) {
      case TokenType::LBrace:
	fiter++;
	tokens.push_back(IniLBrace());
	break;
      case TokenType::Section:
	tokens.push_back(IniSection(read_name(fiter, eos, delimiter)));
	break;
      case TokenType::RBrace:
	fiter++;
	tokens.push_back(IniRBrace());
	break;
      case TokenType::SingleQuote:
	fiter++;
	tokens.push_back(IniSingleQuote());
	break;
      case TokenType::DoubleQuote:
	fiter++;
	tokens.push_back(IniDoubleQuote());
	break;
      case TokenType::String:
	tokens.push_back(IniString(read_name(fiter, eos, delimiter)));
	break;
      case TokenType::Equals:
	fiter++;
	tokens.push_back(IniEquals());
	break;
      case TokenType::Identifier:
	tokens.push_back(IniIdentifier(read_name(fiter, eos, delimiter)));
	break;
      case TokenType::Number:
	tokens.push_back(IniNumber(std::stof(read_number(fiter, eos))));
	break;
      default:
	break;
      }
    }

    // We're done with the stream, so close it to prevent leaks
    // and make it reusable.
    stream_.close();
  }

  /*
   * Peeks in the file stream and determines what the next target is.
   */
  TokenType IniLexer::next_token(IniIterator& fiter,
				 IniIterator& eos,
				 const TokenType previous,
				 Predicate& delimiter) noexcept {
    if (fiter == eos)
      return TokenType::EndOfFile;

    char next = *fiter;

    if (is_whitespace_or_eol(next)) {
      // Discard the whitespace and continue.
      fiter++;
      return next_token(fiter, eos, previous, delimiter);
    }

    if (is_comment(next)) {
      // Skip the comment and continue.
      skip_comment(fiter, eos);
      return next_token(fiter, eos, previous, delimiter);
    }

    switch (previous) {
    case TokenType::LBrace:
      // Previous token == [, look for a section.
      delimiter = make_predicate(']');
      return TokenType::Section;
    case TokenType::Section:
      // Previous token was a section, look for a ].
      delimiter = is_whitespace;
      return TokenType::RBrace;
    case TokenType::DoubleQuote:
      // Previous token and delimiter == "
      // then look for an identifier
      // otherwise look for a string.
      if (!delimiter('"')) {
	delimiter = make_predicate('"');
	return TokenType::String;
      } else {
	delimiter = is_whitespace;
	return TokenType::Identifier;
      }
    case TokenType::SingleQuote:
      // Previous token and delimiter == '
      // then look for an identifier
      // otherwise look for a string.
      if (!delimiter('\'')) {
	delimiter = make_predicate('\'');
	return TokenType::String;
      } else if (next == '[') {
	return TokenType::LBrace;
      } else {
	delimiter = is_whitespace;
	return TokenType::Identifier;
      }
    case TokenType::String:
      // Previous token was a string, look for either ' or ".
      if (delimiter('"'))
	return TokenType::DoubleQuote;
      else
	return TokenType::SingleQuote;
    case TokenType::Identifier:
      // Previous token was an identifier, look for a =.
      return TokenType::Equals;
    case TokenType::Equals:
      // Previous token was =, look for either a number, a string or a bool.
      if (is_numeric(next)) {
	return TokenType::Number;
      } else if (next == '\'') {
	return TokenType::SingleQuote;
      } else if (next == '"') {
	return TokenType::DoubleQuote;
      } else {
	delimiter = is_eol;
	return TokenType::Identifier;
      }
    case TokenType::Null:
      return TokenType::LBrace;
    default:
      // You are probably just looking for an identifier.
      if (next == '[')
	return TokenType::LBrace;
      return TokenType::Identifier;
    }
  }

  /*
   * Skips a single-line comment.
   */
  void IniLexer::skip_comment(IniIterator& fiter, IniIterator& eos) noexcept {
    for (char symbol = *fiter; fiter != eos; symbol = *(++fiter))
      if (is_eol(symbol))
	break;
  }

  /*
   * Reads a 'name' given a predicate to determine whether or not to stop.
   * This is used for sections, identifiers, strings, numbers etc.
   */
  std::string IniLexer::read_name(IniIterator& fiter,
				  IniIterator& eos,
				  Predicate delimiter,
				  Predicate is_ignorable) noexcept {
    std::string name = "";

    for (char symbol = *fiter; fiter != eos && !delimiter(symbol); symbol = *(++fiter))
      if (!is_ignorable(symbol))
	name += symbol;

    return name;
  }

  std::string IniLexer::read_number(IniIterator& fiter, IniIterator& eos) noexcept {
    auto delimiter = compose(is_eol, compose(is_whitespace, [](char c){ return !is_numeric(c); }));
    std::string integer_part = read_name(fiter, eos, delimiter);

    if (*fiter == '.') {
      ++fiter;
      std::string float_part = read_name(fiter, eos, delimiter);
      return integer_part + "." + float_part; 
    }
    
    return integer_part;
  }
};
