#ifndef PARSER_HPP_
#define PARSER_HPP_

#include <concepts>
#include <exception>
#include <future>
#include <iostream>
#include <iterator>
#include <map>
#include <memory_resource>
#include <stdexcept>
#include <string>
#include <thread>
#include <type_traits>
#include <variant>
#include <vector>

#include "lexer.hpp"
#include "tokens.hpp"

namespace libini {

  class IniContainer {
    public:
    IniContainer(IniVariant value) noexcept
      : value_(value) {}

    IniContainer(const IniContainer& other) noexcept
      : value_(other.value_) {}

    IniContainer& operator=(const IniContainer& other) noexcept {
      if (this != &other)
	value_ = other.value_;
      return *this;
    }

    template<typename T>
    requires ParsableToken<T>
    T::value_type get_value() const {
      return std::get<T>(value_).token_value;
    }

    private:
    IniVariant value_;
  };

  class IniParserTree {
    public:
    const std::string get_name() const noexcept;
  };

  class IniParserTreeLeaf : IniParserTree {
    public:
    IniParserTreeLeaf(const std::string name,
		      const IniContainer container) noexcept
      : name_(name), container_(container) {}

    IniParserTreeLeaf(const IniParserTreeLeaf& other) noexcept
      : name_(other.name_), container_(other.container_) {}

    IniParserTreeLeaf& operator=(const IniParserTreeLeaf& other) noexcept {
      if (this != &other) {
	name_ = other.name_;
	container_ = other.container_;
      }

      return *this;
    }

    template<typename T>
    requires ParsableToken<T>
    T::value_type get_value() const {
      return container_.get_value<T>();
    }

    const std::string get_name() const noexcept {
      return name_;
    }

    private:
    std::string name_;
    IniContainer container_;
  };

  class IniParserTreeNode : IniParserTree {
    public:
    IniParserTreeNode(IniSection section) noexcept
      : name_(section.token_value) {}

    IniParserTreeNode(const IniParserTreeNode& other) noexcept
      : name_{other.name_}, children_{other.children_} {}

    IniParserTreeNode& operator=(const IniParserTreeNode& other) noexcept {
      if (this != &other) {
	name_ = other.name_;
	children_ = other.children_;
      }

      return *this;
    }

    IniParserTreeLeaf operator [](const std::string name) const {
      for (auto child : children_) {
	if (child.get_name() == name)
	  return child;
      }

      throw std::runtime_error("libini error: member not found.");
    } 

    bool has_member(const std::string name) const noexcept {
      for (auto child : children_)
	if (child.get_name() == name)
	  return true;

      return false;
    }

    const std::string get_name() const noexcept {
      return name_;
    }

    template<typename T>
    requires ParsableToken<T>
    T::value_type get_value(const std::string name) const {
      auto child = (*this)[name];
      return child.get_value<T>();
    }

    void insert(IniParserTreeLeaf child) noexcept {
      children_.push_back(child);
    }

    private:
    std::string name_;
    std::vector<IniParserTreeLeaf> children_;
  };

  using IniParserRoots = std::pmr::vector<IniParserTreeNode>;
  
  class IniParserResult {
    public:
    IniParserResult(IniParserRoots roots) noexcept
      : roots_(roots) {}
    IniParserResult(const IniParserResult& other) noexcept
      : roots_{other.roots_} {}
    IniParserResult& operator =(const IniParserResult& other) noexcept {
      if (this != &other)
	roots_ = other.roots_;
      return *this;
    }
    ~IniParserResult() noexcept {}

    bool has_member(std::string name) {
      for (auto node : roots_)
	if (node.has_member(name))
	  return true;

      return false;
    }

    IniParserTreeLeaf operator [](const std::string name) const {
      for (auto node : roots_) {
	try {
	  auto child = node[name];
	  return child;
	} catch (std::runtime_error const& ex) {
	  continue;
	}
      }

      throw std::runtime_error("libini error: member not found");
    } 

    template<typename T>
    requires ParsableToken<T>
    T::value_type get_value(const std::string name) const {
      return (*this)[name].get_value<T>();
    }

    private:
    IniParserRoots roots_;
  };

  template<typename LexerType = IniLexer>
  requires IniTokenizer<LexerType>
  class IniParser {
    public:
    IniParser(const std::string file_name) noexcept
      : lexer_{file_name} {}

    IniParserResult operator() () {
      return parse();
    }

    IniParserResult parse() {
      return IniParserResult(build_tree());
    }

    std::future<IniParserResult> parse_async() {
      std::promise<IniParserResult> promise;
      std::future<IniParserResult> f = promise.get_future();
      std::thread([this](auto p){ p.set_value(parse()); }, std::move(promise)).detach();

      return f;
    }

    private:
    LexerType lexer_;

    IniParserRoots build_tree() {
      IniParserRoots roots;
      std::pmr::monotonic_buffer_resource mbr;
      std::pmr::polymorphic_allocator<IniVariant> allocator{&mbr};
      auto tokens = lexer_(allocator);

      parse_section(tokens, roots);

      return roots;
    }

    IniVariant parse_value(auto &tokens, auto &iter) {
      auto type_index = (*iter).index();

      if (type_index >= 8) {
	auto result = *(++iter);
	remove_head(tokens);
	return result;
      } else if (type_index == 1) {
	auto result = *iter;
	remove_head(tokens, 1);
	return result;
      } else throw std::bad_variant_access();
    }

    void parse_member(auto &tokens, auto &node) {
      auto len = tokens.size();

      if (len < 3)
	return;
      
      IniTokens::iterator it = tokens.begin();
      if ((*it).index() == 5)
	      return;

      try {
	auto identifier = std::get<IniIdentifier>(*it);
	auto equals = std::get<IniEquals>(*(++it));
	auto value = parse_value(tokens, (++it));
	auto leaf = IniParserTreeLeaf(identifier.token_value, value);

	remove_head(tokens, 2);
	node.insert(leaf);

	if (tokens.size() > 2)
	  parse_member(tokens, node);

      } catch (std::bad_variant_access const& ex) {
	throw ex;
      }
    } 

    void parse_section(auto &tokens, IniParserRoots &roots) {
      auto len = tokens.size();

      if (len == 0)
	return;

      try {
	auto head = std::get<IniLBrace>(tokens.front()).token_type;

	if (head == TokenType::LBrace) { // Found beginning of section, parse it
	  IniTokens::iterator it = tokens.begin();
	  auto section = std::get<IniSection>(*(++it)); // Get the section
	  remove_head(tokens);

	  auto node = IniParserTreeNode(section);
	
	  parse_member(tokens, node);
	  roots.push_back(node);

	  parse_section(tokens, roots);
	  return;
	} 

	throw std::runtime_error("libini error: parser encountered unexpected token while parsing section.");
      } catch (std::bad_variant_access const& ex) {
	throw std::runtime_error("libini error: parser encountered unexpected token while parsing section.");
      } catch (std::runtime_error const& ex) {
	throw ex;
      }
    }

    void remove_head(auto &tokens, int limit = 3) noexcept {
	for (int i = 0; i < limit; i++) 
	  if (!tokens.empty()) {
	    tokens.pop_front();
	  }
    }
  };
};

#endif
