/** \file parsers.h */   // -*-c++-*-

// Copyright (C) 2010 Daniel Burrows

// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.

// You should have received a copy of the GNU General Public License
// along with this program; see the file COPYING.  If not, write to
// the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA 02111-1307, USA.

#ifndef PARSERS_H
#define PARSERS_H

#include <exception>
#include <string>
#include <sstream>
#include <ostream>
#include <vector>

// A note regarding boost/fusion/include/mpl.hpp: the documentation
// only says this makes MPL sequences into Fusion sequences, but
// according to
// http://archives.free.net.ph/message/20090113.031604.6f18fda2.en.html#boost,
// you also have to include it in order to use Fusion sequences as MPL
// sequences.

#include <boost/format.hpp>
#include <boost/fusion/adapted/mpl.hpp>
#include <boost/fusion/algorithm/iteration/fold.hpp>
#include <boost/fusion/algorithm/iteration/for_each.hpp>
#include <boost/fusion/algorithm/transformation/clear.hpp>
#include <boost/fusion/algorithm/transformation/join.hpp>
#include <boost/fusion/algorithm/transformation/push_back.hpp>
#include <boost/fusion/algorithm/transformation/push_front.hpp>
#include <boost/fusion/container/generation/make_vector.hpp>
#include <boost/fusion/container/list.hpp>
#include <boost/fusion/container/vector.hpp>
#include <boost/fusion/container/vector/convert.hpp>
#include <boost/fusion/functional/adapter/fused.hpp>
#include <boost/fusion/include/join.hpp>
#include <boost/fusion/include/mpl.hpp>
#include <boost/fusion/include/sequence.hpp>
#include <boost/fusion/iterator/equal_to.hpp>
#include <boost/fusion/sequence.hpp>
#include <boost/fusion/support/is_sequence.hpp>
#include <boost/make_shared.hpp>
#include <boost/mpl/begin.hpp>
#include <boost/mpl/fold.hpp>
#include <boost/mpl/front.hpp>
#include <boost/mpl/transform.hpp>
#include <boost/numeric/conversion/cast.hpp>
#include <boost/optional.hpp>
#include <boost/preprocessor/repetition/enum_binary_params.hpp>
#include <boost/preprocessor/repetition/enum_params.hpp>
#include <boost/preprocessor/repetition/repeat.hpp>
#include <boost/range.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/type_traits/is_same.hpp>

#include <generic/util/util.h>

#include <aptitude.h>

#include <errno.h>

namespace parsers
{
  /** \defgroup Parser combinators in C++
   *
   *  \page parser Parsers
   *
   *  A parser provides an operator() method that receives an iterator
   *  range and returns a value of the parser's result_type.  Parsers
   *  are generated from rules; see \ref rule_concept.
   *
   *  Parsers are derived from a specialization of parser_base; this
   *  allows operator overloading to work properly.
   *
   *  Members (see parser_base for more details):
   *
   *  - result_type: the type returned from operator().  Must be default
   *    constructible, assignable, and copy-constructable.
   *
   *  - template<typename ParseInput> parse(ParseInput &input)
   *    const: the actual parse routine.  ParseInput is a parse input
   *    object (see \ref parse_input_concept).  The parse input will
   *    be updated to point to the first character that was not
   *    parsed.  Throws ParseException if the input could not be
   *    parsed.
   *
   *  - template<typename ForwardReadableRange> parse(const
   *    ForwardReadableRange &r) const: a convenience frontend to
   *    parse() that unpacks the given range.
   *
   *  - get_expected_description(std::ostream &out) const: writes a brief description
   *    of the next token expected by this parser to "out".
   *
   *  - derived(): returns a reference to the object, cast to its
   *    concrete parser type.
   *
   *  \note Parsers are not specialized on the character type of their
   *  input stream; the character type is simply the value_type of the
   *  iterator passed to operator().
   *
   *  \page rule_concept Rule concept
   *
   *  A rule is the code that defines a parser (see \ref parser).
   *  Rules must derive from a parser_base class instantiated with
   *  their own class type and their return type.
   *
   *  Members required:
   *
   *  - result_type: the type returned from operator().  This is
   *    inherited from parser_base.  However, some rules choose to
   *    define it themselves because otherwise it's not available in
   *    the rule class's definition (for technical reasons).
   *
   *  - template<typename ParseInput> apply(ParseInput &input)
   *    const: the actual parse routine.  ParseInput is a parse input
   *    object (see \ref parse_input_concept).  The parse input will
   *    be updated to point to the first character that was not
   *    parsed.  Throws ParseException if the input could not be
   *    parsed.
   *
   *  - get_expected(std::ostream &out) const: writes a brief description
   *    of the next token expected by this rule to "out".
   *
   *  \page parse_input_concept ParseInput concept
   *
   *  The ParseInput concept represents objects that provides access
   *  to an input range for the parser, and also handles keeping track
   *  of the current line and column number.
   *
   *  An object modeling ParseInput provides the following members:
   *
   *  - ParseInput(ParseInput &): copy-constructor.
   *
   *  - typedef (...) value_type: the type of object that the input
   *                              stream consists of.
   *
   *  - typedef (...) const_iterator: a model of ForwardIterator.
   *
   *  - bool empty() const: test whether there is any more input in
   *                        the stream.
   *
   *  - value_type front() const: if the stream is not empty, return
   *                              its first element.
   *
   *  - const_iterator begin() const: returns an iterator to the
   *                                  current stream position.  Used
   *                                  to check whether a sub-parser
   *                                  consumed anything, and to report
   *                                  how much was parsed to client
   *                                  code.
   *
   *  - const std::string &getFilename() const: retrieve the file name
   *                                            of the current input
   *                                            position.
   *
   *  - int getLineNumber() const: retrieve the line number of the
   *                               current input position.
   *
   *  - int getColumnNumber() const: retrieve the column number of the
   *                                 current input position.
   *
   *  - void setFilename(const std::string &): set the filename of the
   *                                           current input position.
   *
   *  - void setLineNumber(int): set the line number of the current
   *                             input position.
   *
   *  - void setColumnNumber(int): set the column number of the
   *                               current input position.
   *
   *  - void fail(const std::string &msg) const:
   *     throw a ParseException initialized with the current input state.
   *     This routine should have __attribute__ ((noreturn)) so the compiler
   *     knows it doesn't return.
   *
   *  - void advance(): if the stream is nonempty, advance to the next
   *                    input location and update the internal
   *                    accounting accordingly.
   *
   *  The lack of an end() is deliberate: begin() isn't supposed to be
   *  used to iterate over the sequence (since then you lose the
   *  benefit of keeping track of the current position) and so end()
   *  is hidden to keep absent-minded coders from accidentally
   *  iterating with begin().
   */

  /** \brief Exception thrown for parse errors. */
  class ParseException : public std::exception
  {
    std::string msg;
    std::string raw_msg;
    std::string filename;
    int lineNumber;
    int columnNumber;

    static std::string show_filename(const std::string &input_filename)
    {
      if(input_filename.empty())
        return "<none>";
      else
        return input_filename;
    }

  public:
    /** \brief Create a new parse exception.
     *
     *  \param _msg  The error message attached to this exception.
     *  \param _filename     The filename in which the error occurred, or
     *                       an empty string for an anonymous file.
     *  \param _lineNumber   The line number in which the error occurred.
     *  \param _columnNumber The column number in which the error occurred.
     */
    ParseException(const std::string &_msg,
                   const std::string &_filename,
                   int _lineNumber,
                   int _columnNumber)
      : msg( (boost::format("%s:%d:%d: %s") % show_filename(_filename) % _lineNumber % _columnNumber % _msg).str() ),
        raw_msg(msg),
        filename(_filename),
        lineNumber(_lineNumber),
        columnNumber(_columnNumber)
    {
    }

    /** \brief Retrieve the raw error message, without the filename
     *  and line/column numbers.
     */
    const std::string &get_raw_msg() const { return raw_msg; }

    /** \brief Retrieve the filename in which the error occurred. */
    const std::string &get_filename() const { return filename; }

    /** \brief Retrieve the line number on which the error occurred. */
    int get_line_number() const { return lineNumber; }

    /** \brief Retrieve the column number in which the error occurred. */
    int get_column_number() const { return columnNumber; }

    ~ParseException() throw()
    {
    }

    const char *what() throw()
    {
      return msg.c_str();
    }
  };

  /** \brief Abstraction used to represent the input stream.
   *
   *  Using this instead of raw iterators allows us to encapsulate
   *  line and character computation.  Normally only one of these is
   *  instantiated at the highest level of a parse.
   *
   *  Models the \ref parse_input_concept.
   *
   *  \tparam ForwardReadableRange The ForwardReadableRange type
   *  wrapped by this ParseInput (see Boost.Range).
   */
  template<typename ForwardReadableRange>
  class parse_input
  {
  public:
    typedef typename boost::range_iterator<ForwardReadableRange>::type const_iterator;
    typedef typename const_iterator::value_type value_type;

  private:
    const_iterator beginIt, end;
    std::string filename;
    int lineNumber, columnNumber;

  public:
    /** \brief Create a new parse-input object.
     *
     *  \param r The range to process.  Iterators into the given
     *  object will be stored, so the new parse_input object should
     *  not be used after the range it was constructed from has been
     *  destroyed.
     *
     *  \param _filename The filename that should be used to generate
     *                   error messages; if empty or omitted, the file
     *                   is assumed to be anonymous.
     *
     *  \param _lineNumber   The initial line number of this input object.
     *  \param _columnNumber The initial column number of this input object.
     */
    parse_input(const ForwardReadableRange &r, const std::string &_filename = std::string(),
                int _lineNumber = 1, int _columnNumber = 1)
      : beginIt(r.begin()),
        end(r.end()),
        filename(_filename),
        lineNumber(_lineNumber),
        columnNumber(_columnNumber)
    {
    }

    /** \brief Test whether this range is empty. */
    bool empty() const { return beginIt == end; }

    /** \brief Retrieve the first element of a non-empty range. */
    value_type front() const
    {
      if(beginIt == end)
        fail("Internal error: accessing past end of input.");

      return *beginIt;
    }

    /** \brief Retrieve an iterator to the first element.
     *
     *  \note This should only be used for two purposes: first, to
     *  report to the caller how far "begin" moved; and second, to
     *  test whether "begin" was moved by a sub-parser.
     */
    const_iterator begin() const
    {
      return beginIt;
    }

    /** \brief Get the current file name. */
    const std::string &getFilename() const { return filename; }

    /** \brief Get the current line number. */
    int getLineNumber() const { return lineNumber; }

    /** \brief Get the current column number. */
    int getColumnNumber() const { return columnNumber; }

    /** \brief Set the current file name. */
    void setFilename(const std::string &newFilename) { filename = newFilename; }

    /** \brief Set the current line number. */
    void setLineNumber(int newLineNumber) { lineNumber = newLineNumber; }

    /** \brief Set the current column number. */
    void setColumnNumber(int newColumnNumber) { columnNumber = newColumnNumber; }

    /** \brief Throw a ParseException that includes information about
     *  the current input position.
     */
    void fail(const std::string &msg) const __attribute__((noreturn))
    {
      throw ParseException(msg, filename, lineNumber, columnNumber);
    }

    /** \brief Advance to the next character of the input. */
    void advance()
    {
      if(beginIt == end)
        fail("Internal error: advancing past end of input.");

      // Update the count of lines and columns.  Should tabs be
      // handled specially?
      switch(*beginIt)
        {
        case '\n':
          ++lineNumber;
          columnNumber = 1;
          break;

        default:
          ++columnNumber;
          break;
        }

      ++beginIt;
    }
  };

  /** \brief Used when a parser has no sensible return value. */
  class nil_t { };

  template<typename P>
  class set_expected_p;

  /** \brief The base class for all parser objects.
   *
   *  Having a common base for parsers allows us to do operator
   *  overloading more sanely.  In the future, it could also be a
   *  locus for various sorts of frontend activities (e.g., accepting
   *  a Boost-style range instead of two iterators).
   *
   *  This class is not copy-constructible, default-constructible, or
   *  destructible, but its subclasses are.  This is because you
   *  should always instantiate a subclass, not one of these; treat
   *  parser_base as an abstract class.
   *
   *  \todo It's not clear that the Curious Template Pattern does much
   *  for us here; just having an empty base class might be equally
   *  useful.
   *
   *  \tparam DerivedT   The derived type that actually implements the parser's
   *                     behavior.  Must be a subclass of this instantiation
   *                     of parser_base.
   *  \tparam ResultType The type returned by the parse operation.
   *                     Must be default constructable, assignable,
   *                     and copy-constructable.
   */
  template<typename DerivedT, typename ResultType>
  class parser_base
  {
  protected:
    parser_base()
    {
    }

    parser_base(const parser_base &other)
    {
    }

    ~parser_base()
    {
    }

  public:
    typedef ResultType result_type;

    // These names are provided both here *and* in the derived class
    // to allow operators to accept a parser_base<> and still get
    // compile-time resolution of the parse functionality (well, sort
    // of).

    /** \brief Parse a range of text.
     *
     *  \tparam Iter   A model of ForwardIterator used to store
     *                 iterators over the input text.
     *
     *  \param begin   The start of the range of text to parse.
     *                 Will be updated to point past whatever
     *                 was parsed.
     *
     *  \param end     The end of the range of text to parse.
     *
     *  \return the parsed value.
     *
     *  \throw ParseException if the parse fails.
     */
    template<typename Iter>
    result_type parse(Iter &begin, const Iter &end) const
    {
      parse_input<boost::iterator_range<Iter> > input(boost::make_iterator_range(begin, end));

      try
        {
          result_type rval = parse(input);
          begin = input.begin();
          return rval;
        }
      catch(ParseException &)
        {
          // Ensure that the caller gets to see changes in "begin".
          // (is this really required?)
          begin = input.begin();
          throw;
        }
    }

    /** \brief Parse a range of text.
     *
     *  \tparam ForwardReadableRange  The type of the input range.
     *
     *  \param r       The range to parse.
     *
     *  \return the parsed value.
     *
     *  \throw ParseException if the parse fails.
     */
    template<typename ForwardReadableRange>
    result_type parse(const ForwardReadableRange &r) const
    {
      parse_input<ForwardReadableRange> input(r);
      return parse(input);
    }

    /** \brief Parse a parse input object.
     *
     *  \tparam ParseInput  The model of ParseInput that is passed
     *                      into this method.
     *  \param input        The input to parse.
     */
    template<typename ParseInput>
    result_type parse(ParseInput &input) const
    {
      return derived().apply(input);
    }

    /** \brief Write a description of what we expect to see here to
     *  the given stream.
     */
    void get_expected_description(std::ostream &out) const
    {
      derived().get_expected(out);
    }

    /** \brief Get a reference to this object, cast to its derived type.
     *
     *  We need a reference to the derived type whenever this is going
     *  to be stored (since you shouldn't store parser_base by
     *  accident).
     */
    DerivedT &derived()
    {
      return *static_cast<DerivedT *>(*this);
    }

    /** \brief Get a reference to this object, cast to its derived type.
     *
     *  We need a reference to the derived type whenever this is going
     *  to be stored (since you shouldn't store parser_base by
     *  accident).
     */
    const DerivedT &derived() const
    {
      return *static_cast<const DerivedT *>(this);
    }

    /** \brief Create a parser that modifies the expected value of
     *  this parser.
     */
    set_expected_p<DerivedT> operator[](const std::string &msg) const
    {
      return set_expected_p<DerivedT>(derived(), msg);
    }
  };

  /** \brief Metafunction class to retrieve the return type of a parser. */
  struct get_result_type
  {
    template<typename P>
    struct apply
    {
      typedef typename P::result_type type;
    };
  };

  /** \brief Atomic parsers */
  // @{

  /** \brief Parse only the given character.
   */
  template<typename CType>
  class ch_p : public parser_base<ch_p<CType>, CType >
  {
    CType c;

  public:
    ch_p(CType _c)
      : c(_c)
    {
    }

    typedef typename parser_base<ch_p<CType>, CType>::result_type result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      if(input.empty())
        input.fail((boost::format(_("Expected '%s', but got EOF.")) % c).str());
      else if(input.front() != c)
        input.fail((boost::format(_("Expected '%s', but got '%s'.")) % c % input.front()).str());
      else
        {
          input.advance();
          return c;
        }
    }

    void get_expected(std::ostream &out) const
    {
      out << "'" << c << "'";
    }
  };

  /** \brief Create a parser that accepts only the given character. */
  inline ch_p<char> ch(char c) { return ch_p<char>(c); }
  /** \brief Create a parser that accepts only the given character. */
  inline ch_p<unsigned char> uch(unsigned char c) { return ch_p<unsigned char>(c); }
  /** \brief Create a parser that accepts only the given character. */
  inline ch_p<signed char> sch(signed char c) { return ch_p<signed char>(c); }

  /** \brief Create a parser that accepts only the given character. */
  inline ch_p<wchar_t> wch(wchar_t c) { return ch_p<wchar_t>(c); }

  /** \brief Parse any character. */
  template<typename CType>
  class anychar_p : public parser_base<anychar_p<CType>, CType >
  {
  public:
    anychar_p()
    {
    }

    template<typename ParseInput>
    CType apply(ParseInput &input) const
    {
      BOOST_STATIC_ASSERT( (boost::is_same<typename ParseInput::value_type, CType>::value) );

      if(input.empty())
        {
          std::ostringstream msg;
          msg << _("Expected any character, but got EOF.");
          input.fail(msg.str());
        }
      else
        {
          CType rval = input.front();
          input.advance();
          return rval;
        }
    }

    void get_expected(std::ostream &out) const
    {
      out << _("any character");
    }
  };

  /** \brief Create a parser that accepts any character. */
  inline anychar_p<char> anychar() { return anychar_p<char>(); }

  /** \brief Create a parser that accepts any unsigned char. */
  inline anychar_p<unsigned char> anyuchar() { return anychar_p<unsigned char>(); }

  /** \brief Create a parser that accepts any signed char. */
  inline anychar_p<signed char> anyschar() { return anychar_p<signed char>(); }

  /** \brief Create a parser that accepts any wide character. */
  inline anychar_p<wchar_t> anywchar() { return anychar_p<wchar_t>(); }



  /** \brief Create a parser that accepts any character passing a
   *  predicate.
   */
  template<typename CType, typename F>
  class charif_p : public parser_base<charif_p<CType, F>, CType>
  {
    F f;
    std::string description;

  public:
    charif_p(const F &_f, const std::string &description)
      : f(_f)
    {
    }

    typedef CType result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      BOOST_STATIC_ASSERT( (boost::is_same<typename ParseInput::value_type, CType>::value) );

      if(input.empty())
        input.fail((boost::format(_("Expected %s, but got EOF.")) % description).str());
      else
        {
          CType c(input.front());

          if(f(c))
            {
              input.advance();
              return c;
            }
          else
            input.fail((boost::format(_("Expected %s, but got '%c'.")) % description % c).str());
        }
    }

    void get_expected(std::ostream &out) const
    {
      out << description;
    }
  };

  /** \brief Parsers for character classes.
   *
   *  These use the standard C character classes.  Arguably these
   *  should be implemented by hand instead, as the standard character
   *  classes are locale-dependent.  However, aptitude's existing
   *  parsers use character classes, so this is not a regression (file
   *  under "don't try to fix the whole world at once").
   */

  // @{

  class alnum_f
  {
  public:
    bool operator()(char c) const { return isalnum(c); }
  };
  typedef charif_p<char, alnum_f> alnum_p;
  /** \brief Create a parser that accepts only letters and numbers. */
  inline alnum_p alnum() { return alnum_p(alnum_f(), "letter or digit"); }


  class alpha_f
  {
  public:
    bool operator()(char c) const { return isalpha(c); }
  };
  typedef charif_p<char, alpha_f> alpha_p;
  /** \brief Create a parser that accepts only letters. */
  inline alpha_p alpha() { return alpha_p(alpha_f(), "letter"); }


  class ascii_f
  {
  public:
    bool operator()(char c) const { return isascii(c); }
  };
  typedef charif_p<char, ascii_f> ascii_p;
  inline ascii_p ascii() { return ascii_p(ascii_f(), "an ASCII character"); }


  class blank_f
  {
  public:
    bool operator()(char c) const { return isblank(c); }
  };
  typedef charif_p<char, blank_f> blank_p;
  /** \brief Create a parser that accepts only blank characters. */
  inline blank_p blank() { return blank_p(blank_f(), "a blank"); }


  class cntrl_f
  {
  public:
    bool operator()(char c) const { return iscntrl(c); }
  };
  typedef charif_p<char, cntrl_f> cntrl_p;
  /** \brief Create a parser that accepts only control characters. */
  inline cntrl_p cntrl() { return cntrl_p(cntrl_f(), "a control character"); }


  class digit_f
  {
  public:
    bool operator()(char c) const { return isdigit(c); }
  };
  typedef charif_p<char, digit_f> digit_p;
  /** \brief Create a parser that accepts only digits. */
  inline digit_p digit() { return digit_p(digit_f(), "a digit"); }


  class graph_f
  {
  public:
    bool operator()(char c) const { return isgraph(c); }
  };
  typedef charif_p<char, graph_f> graph_p;
  /** \brief Create a parser that accepts only printable characters
   *  that aren't space.
   */
  inline graph_p graph() { return graph_p(graph_f(), "a visible character"); }


  class lower_f
  {
  public:
    bool operator()(char c) const { return islower(c); }
  };
  typedef charif_p<char, lower_f> lower_p;
  /** \brief Create a parser that accepts only lower-case characters.
   */
  inline lower_p lower() { return lower_p(lower_f(), "a lower-case letter"); }


  class print_f
  {
  public:
    bool operator()(char c) const { return isprint(c); }
  };
  typedef charif_p<char, print_f> print_p;
  /** \brief Create a parser that accepts only printable characters. */
  inline print_p print() { return print_p(print_f(), "a printable character"); }


  class punct_f
  {
  public:
    bool operator()(char c) const { return ispunct(c); }
  };
  typedef charif_p<char, punct_f> punct_p;
  /** \brief Create a parser that accepts only punctuation characters. */
  inline punct_p punct() { return punct_p(punct_f(), "punctuation"); }


  class space_f
  {
  public:
    bool operator()(char c) const { return isspace(c); }
  };
  typedef charif_p<char, space_f> space_p;
  /** \brief Create a parser that accepts only whitespace characters. */
  inline space_p space() { return space_p(space_f(), "whitespace"); }


  class upper_f
  {
  public:
    bool operator()(char c) const { return isupper(c); }
  };
  typedef charif_p<char, upper_f> upper_p;
  /** \brief Create a parser that accepts only upper-case
   *  characters.
   */
  inline upper_p upper() { return upper_p(upper_f(), "an upper-case letter"); }


  class xdigit_f
  {
  public:
    bool operator()(char c) const { return isxdigit(c); }
  };
  typedef charif_p<char, xdigit_f> xdigit_p;
  /** \brief Create a parser that accepts only hexadecimal digits. */
  inline xdigit_p xdigit() { return xdigit_p(xdigit_f(), "a hexadecimal digit"); }

  // @}

  /** \brief A parser for integer values.
   *
   *  Stops at the first non-digit; it's up to the client to require
   *  trailing whitespace as necessary.
   *
   *  Note: this is locale-dependent.  Should it be changed to not be?
   */
  class integer_p : public parser_base<integer_p, int>
  {
  public:
    typedef int result_type;

    template<typename ParseInput>
    int apply(ParseInput &input) const
    {
      BOOST_STATIC_ASSERT( (boost::is_same<char, typename ParseInput::value_type>::value) );

      if(input.empty())
        input.fail(_("Expected an integer, got EOF."));

      typename ParseInput::const_iterator start = input.begin();

      // First, parse the sign (if any).
      if(input.front() == '-')
        input.advance();

      if(input.empty())
        input.fail(_("Expected an integer following '-', got EOF."));

      while(!input.empty() && isdigit(input.front()))
        input.advance();

      if(input.begin() == start)
        input.fail((boost::format(_("Expected an integer, got '%c'.")) % input.front()).str());

      // Lean on strtol for now.
      std::string s(start, input.begin());
      char *endptr;
      errno = 0;
      long rval = strtol(s.c_str(), &endptr, 0);
      if(errno != 0)
        {
          int errnum = errno;
          input.fail(sstrerror(errnum));
        }

      if(*endptr != '\0')
        input.fail((boost::format(_("Invalid integer: \"%s\".")) % s).str());

      try
        {
          return boost::numeric_cast<int>(rval);
        }
      catch(boost::bad_numeric_cast &ex)
        {
          input.fail((boost::format(_("Invalid integer: \"%s\".")) % ex.what()).str());
        }
    }

    void get_expected(std::ostream &out) const
    {
      out << "integer";
    }
  };

  /** \brief Create a parser that recognizes integers. */
  inline integer_p integer() { return integer_p(); }

  /** \brief A parser that recognizes EOF. */
  class eof : public parser_base<eof, nil_t>
  {
  public:
    typedef nil_t result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      if(!input.empty())
        input.fail((boost::format(_("Expected EOF, got '%c'.")) % input.front()).str());

      return nil_t();
    }

    void get_expected(std::ostream &out) const
    {
      out << "EOF";
    }
  };

  /** \brief A parser that recognizes a particular string.
   *
   *  \note For the sake of efficiency, returns nothing (normally this
   *  is invoked with a constant string and the caller doesn't care
   *  about getting it back).
   */
  class str : public parser_base<str, nil_t>
  {
    std::string s;
  public:
    explicit str(const std::string &_s)
      : s(_s)
    {
    }

    typedef nil_t result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      typename ParseInput::const_iterator start = input.begin();

      std::string::const_iterator s_begin(s.begin()), s_end(s.end());

      while(!input.empty() && s_begin != s_end)
        {
          if(input.front() != *s_begin)
            input.fail((boost::format("Expected \"%s\", but got '%c' following \"%s\".")
                        % s % input.front() % std::string(start, input.begin())).str());

          input.advance();
          ++s_begin;
        }

      if(s_begin != s_end)
        input.fail((boost::format("Expected \"%s\", but got EOF following \"%s\".")
                    % s % std::string(start, input.begin())).str());

      return nil_t();
    }

    void get_expected(std::ostream &out) const
    {
      out << '"' << s << '"';
    }
  };


  /** \brief A parser that parses nothing and returns a fixed value.
   *
   *  \tparam T   The value type to return; must be CopyConstructible.
   */
  template<typename T>
  class val_p : public parser_base<val_p<T>, T>
  {
    T value;

  public:
    val_p(const T &_value)
      : value(_value)
    {
    }

    typedef T result_type;

    template<typename ParseInput>
    T apply(ParseInput &) const
    {
      return value;
    }

    void get_expected(std::ostream &out) const
    {
      out << _("anything");
    }
  };

  /** \brief Create a parser that parses nothing and returns a value. */
  template<typename T>
  inline val_p<T> val(const T &value)
  {
    return val_p<T>(value);
  }

  /** \brief Create a parser that parses nothing and returns a string.
   *
   *  This overload is purely so val("abc") works; otherwise the
   *  compiler tries to save an array or a char pointer.
   */
  inline val_p<std::string> val(const char *s)
  {
    return val_p<std::string>(s);
  }

  // @}

  /** \brief Combine two parsers in sequence, throwing away the first
   *  parser's result.
   *
   *  \todo Implement this using Boost.Fusion containers, like or_p.
   */
  template<typename P1, typename P2>
  class andthen_p : public parser_base<andthen_p<P1, P2>, typename P2::result_type>
  {
    P1 p1;
    P2 p2;

  public:
    andthen_p(const P1 &_p1, const P2 &_p2)
      : p1(_p1), p2(_p2)
    {
    }

    typedef typename P2::result_type result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      p1.parse(input);
      return p2.parse(input);
    }

    void get_expected(std::ostream &out) const
    {
      p1.get_expected_description(out);
    }
  };

  /** \brief Combine two parsers in sequence, throwing away the first
   *  parser's result.
   */
  template<typename Rule1, typename ResultType1, typename Rule2, typename ResultType2>
  andthen_p<Rule1, Rule2>
  inline operator>>(const parser_base<Rule1, ResultType1> &p1, const parser_base<Rule2, ResultType2> &p2)
  {
    return andthen_p<Rule1, Rule2>(p1.derived(), p2.derived());
  }

  /** \brief Combine two parsers in sequence, throwing away the second
   *  parser's result.
   *
   *  \todo Implement this using Boost.Fusion containers, like or_p.
   */
  template<typename P1, typename P2>
  class andfirst_p : public parser_base<andfirst_p<P1, P2>, typename P1::result_type>
  {
    P1 p1;
    P2 p2;

  public:
    andfirst_p(const P1 &_p1, const P2 &_p2)
      : p1(_p1), p2(_p2)
    {
    }

    typedef typename P1::result_type result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      result_type rval = p1.parse(input);
      p2.parse(input);
      return rval;
    }

    void get_expected(std::ostream &out) const
    {
      p1.get_expected_description(out);
    }
  };

  /** \brief Combine two parsers in sequence, throwing away the second
   *  parser's result.
   *
   *  Mnemonic: the arrow points at the value that's returned.
   */
  template<typename Rule1, typename ResultType1, typename Rule2, typename ResultType2>
  andfirst_p<Rule1, Rule2>
  inline operator<<(const parser_base<Rule1, ResultType1> &p1, const parser_base<Rule2, ResultType2> &p2)
  {
    return andfirst_p<Rule1, Rule2>(p1.derived(), p2.derived());
  }

  /** \brief A parser that modifies the expected value of its target. */
  template<typename P>
  class set_expected_p : public parser_base<set_expected_p<P>, typename P::result_type>
  {
    P p;
    std::string msg;

  public:
    set_expected_p(const P &_p, const std::string &_msg)
      : p(_p), msg(_msg)
    {
    }

    typedef typename P::result_type result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      return p.parse(input);
    }

    void get_expected(std::ostream &out) const
    {
      out << msg;
    }
  };

  /** \brief Parse zero or more occurrences of the input pattern,
   *  invoking a function object for each occurrence.
   *
   *  Has no return value (it returns nil_t).  If the functor throws a
   *  ParseException, this parser will fail with that exception.
   */
  template<typename P, typename F>
  class foreach_p : public parser_base<foreach_p<P, F>, nil_t>
  {
    P p;
    F f;

  public:
    foreach_p(const P &_p, const F &_f)
      : p(_p), f(_f)
    {
    }

    typedef nil_t result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      while(true)
        {
          typename P::result_type result;

          try
            {
              result = p.parse(input);
            }
          catch(ParseException &)
            {
              break;
            }

          f(result);
        }

      return nil_t();
    }

    void get_expected(std::ostream &out) const
    {
      p.get_expected(out);
    }
  };

  /** \brief Parse zero of more occurrences of the input pattern,
   *  invoking a function object for each occurrence.
   */
  template<typename P, typename F>
  inline foreach_p<P, F> foreach(const P &p, const F &f)
  {
    return foreach_p<P, F>(p, f);
  }

  /** \brief Useful functors. */
  // @{

  /** \brief Push incoming values onto an STL-style sequence. */
  template<typename C>
  class push_back_f
  {
    C &c;

  public:
    /** \brief Construct a push-back functor for the given container. */
    push_back_f(C &_c)
      : c(_c)
    {
    }

    template<typename V>
    void operator()(const V &v) const
    {
      c.push_back(v);
    }
  };

  /** \brief Push incoming values onto an STL-style sequence. */
  template<typename C>
  push_back_f<C> push_back_a(C &c)
  {
    return push_back_f<C>(c);
  }

  // @}

  /** \brief Parse zero or more occurrences of the input pattern,
   *  ignoring the resulting values.
   */
  template<typename P>
  class skip_p : public parser_base<skip_p<P>, nil_t>
  {
    P p;
  public:

    skip_p(const P &_p)
      : p(_p)
      {
      }

    typedef nil_t result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      while(true)
        {
          typename ParseInput::const_iterator where = input.begin();
          try
            {
              p.parse(input);
            }
          catch(ParseException &)
            {
              if(input.begin() != where)
                throw;
              break;
            }
        }

      return nil_t();
    }
  };

  /** \brief Create a parser that skips zero or more copies of the
   *  input parser.
   */
  template<typename P>
  inline skip_p<P> skip(const P &p)
  {
    return skip_p<P>(p);
  }

  /** \brief A parser that applies a sub-parser one or more times,
   *  discarding its parses.
   *
   *  \note Implemented this way and not as a convenience wrapper
   *  because the convenience wrapper would store two copies of the
   *  parser P.
   */
  template<typename P>
  class skipOne_p : public parser_base<skipOne_p<P>, nil_t>
  {
    P p;

  public:
    skipOne_p(const P &_p) : p(_p) { }

    template<typename ParseInput>
    nil_t apply(ParseInput &input) const
    {
      p.parse(input);

      while(true)
        {
          typename ParseInput::const_iterator where = input.begin();

          try
            {
              p.parse(input);
            }
          catch(ParseException &)
            {
              if(where != input.begin())
                throw;

              break;
            }
        }

      return nil_t();
    }
  };

  /** \brief Create a parser that applies the given parser one
   *  or more times.
   *
   *  Stops when the sub-parser fails; if it fails without consuming
   *  input after producing at least one result, this parser succeeds.
   */
  template<typename P>
  skipOne_p<P> skipOne(const P &p)
  {
    return skipOne_p<P>(p);
  }

  /** \brief A parser that applies a sub-parser zero or more times,
   *  collecting its parses.
   *
   *  \tparam P  The sub-parser.
   *  \tparam Container  The container type to return.  e.g., change
   *                     to std::string to produce a string value.
   *
   *  The returned container is wrapped in a shared_ptr to avoid
   *  unnecessary copies.
   *
   *  Note that if the sub-parser fails after consuming input, this
   *  parser will fail as well (since otherwise we would leave the
   *  parse in a bad state).
   */
  template<typename P, typename Container = std::vector<typename P::result_type> >
  class many_p : public parser_base<many_p<P, Container>, boost::shared_ptr<Container> >
  {
    P p;
  public:
    many_p(const P &_p)
      : p(_p)
    {
    }

    typedef boost::shared_ptr<Container> result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      boost::shared_ptr<Container> rval =
        boost::make_shared<Container>();

      while(true)
        {
          typename ParseInput::const_iterator where = input.begin();
          try
            {
              rval->push_back(p.parse(input));
            }
          catch(ParseException &ex)
            {
              if(where != input.begin())
                throw;
              break;
            }
        }

      return rval;
    }
  };

  // Sanity-check that we get the correct result type out of a
  // many_p expression.
  BOOST_STATIC_ASSERT((boost::is_same<many_p<integer_p, std::vector<integer_p::result_type> >::result_type,
                                      boost::shared_ptr<std::vector<int> > >::value));

  /** \brief Apply the input parser zero or more times, saving
   *  the results to a vector.
   */
  template<typename P>
  inline many_p<P> many(const P &p)
  {
    return many_p<P>(p);
  }

  /** \brief Apply the input parser zero or more times, saving
   *  the results to a string.
   *
   *  \note The input parser must have a return type of char.
   */
  template<typename P>
  inline many_p<P, std::string> many_string(const P &p)
  {
    return many_p<P, std::string>(p);
  }

  /** \brief A parser that recognizes zero or more occurrences of its
   *  sub-parser, separated by occurrences of a separator parser.  The
   *  separator's return values are discarded.
   *
   *  \tparam SeparatorP  A parser type that will recognize the
   *                      separators between occurrences of ValueP.
   *  \tparam ValueP A parser type that will recognize the values that
   *                 this parser returns.
   *  \tparam Seq    The sequence type used to store the results of
   *                 the ValueP parser. 
  *
   *  The parser terminates successfully when SeparatorP matches a
   *  zero-length value.  Any other condition (ValueP failing to match
   *  or SeparatorP failing and consuming input) will result in a
   *  parse failure.
   */
  template<typename SeparatorP, typename ValueP,
	   typename Seq = std::vector<typename ValueP::result_type> >
  class sepBy_p : public parser_base<sepBy_p<SeparatorP, ValueP, Seq>,
				     boost::shared_ptr<Seq> >
  {
    SeparatorP separatorP;
    ValueP valueP;

  public:
    sepBy_p(const SeparatorP &_separatorP, const ValueP &_valueP)
      : separatorP(_separatorP), valueP(_valueP)
    {
    }

    typedef boost::shared_ptr<Seq> result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      result_type rval = boost::make_shared<Seq>();

      bool first = true;
      while(true)
      {
	typename ParseInput::const_iterator initialBegin = input.begin();

	try
	  {
	    if(first)
	      first = false;
	    else
	      separatorP.parse(input);

	    rval->push_back(valueP.parse(input));
	  }
	catch(ParseException &)
	  {
	    if(input.begin() == initialBegin)
	      break;
	    else
	      throw;
	  }
      }

      return rval;
    }
  };

  /** \brief Create a parser that recognizes zero or more occurrences
   *  of its sub-parser, with occurrences of a separator parser
   *  between them (but not at the beginning or end of the match).
   *
   *  \tparam ValueP A parser type that will recognize the values that
   *                 this parser returns.
   *  \tparam SeparatorP  A parser type that will recognize the
   *                      separators between occurrences of ValueP.
   *
   *  \param separatorP The parser for the tokens that separate
   *                     values.  The values returned by this parser
   *                     are discarded.
   *
   *  \param valueP The parser that generates the values that are to
   *                 be returned.
   */
  template<typename SeparatorP, typename ValueP>
  sepBy_p<SeparatorP, ValueP> sepBy(const SeparatorP &separatorP,
				    const ValueP &valueP)
  {
    return sepBy_p<SeparatorP, ValueP>(separatorP, valueP);
  }


  /** \brief A parser that applies a sub-parser and checks its output
   *  against a post-condition, returning a result only if the output
   *  passes a test.
   *
   *  \tparam P  The parser type to wrap.
   *  \tparam F  The function object type of the assertion.
   */
  template<typename P, typename F>
  class assert_p : public parser_base<assert_p<P, F>,
                                      typename P::result_type>
  {
    P p;
    F f;
    std::string expected;

  public:
    /** \brief Create a new assertion parser.
     *
     *  \param _p The parser to wrap.
     *  \param _f The assertion to test.
     *  \param _expected A description of what this assertion expects.
     */
    assert_p(const P &_p, const F &_f, const std::string &_expected)
      : p(_p), f(_f), expected(_expected)
    {
    }

    typedef typename P::result_type result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      result_type rval = p.parse(input);

      if(!f(rval))
        input.fail((boost::format(_("Expected %s")) % expected).str());

      return rval;
    }

    void get_expected(std::ostream &out) const
    {
      out << expected;
    }
  };

  /** \brief Create a parser that applies a sub-parser and checks its
   *  output against a post-condition, returning a result only if the
   *  output passes a test.
   *
   *  \tparam P  The parser type to wrap.
   *  \tparam F  The function object type of the assertion.
   *
   *  \param p The parser to wrap.
   *  \param expected The error message to throw if the assert fails.
   *  \param f The assertion to test (defaults to F()).
   */
  template<typename P, typename F>
  assert_p<P, F> postAssert(const P &p, const std::string &expected, const F &f = F())
  {
    return assert_p<P, F>(p, f, expected);
  }

  /** \brief Given a Boost.Fusion sequence, try each of the parsers it
   *  contains in turn.
   *
   *  If the first parser fails after consuming no input, the second
   *  parser is tried.  If the first parser fails after consuming some
   *  input, this parser fails.  If none of the parsers succeed, this
   *  parser fails.
   *
   *  The result_type of or_p is the result_type of the first parser.
   *  The parsers that follow it must be convertible to the same
   *  result_type.
   */
  template<typename C>
  class or_p : public parser_base<or_p<C>, typename boost::mpl::front<C>::type::result_type>
  {
  public:
    typedef typename boost::mpl::front<C>::type::result_type result_type;

  private:
    // Metaprogramming to verify that all the sub-types are the same.
    typedef typename boost::mpl::transform<C, get_result_type>::type C_result_types;
    typedef typename boost::mpl::transform<C_result_types,
                                           boost::is_same<result_type,
                                                          boost::mpl::_1> >::type C_result_types_equal_to_front;
    typedef typename boost::mpl::fold<C_result_types_equal_to_front,
                                      boost::mpl::true_,
                                      boost::mpl::and_<boost::mpl::_1, boost::mpl::_2> >::type all_subtypes_are_equal;

    BOOST_STATIC_ASSERT(all_subtypes_are_equal::value);

    C values;

    class do_get_expected
    {
      std::ostream &out;
      bool &first;

    public:
      do_get_expected(std::ostream &_out, bool &_first)
        : out(_out),
          first(_first)
      {
      }

      template<typename Rule>
      void operator()(const Rule &r) const
      {
        if(!first)
          out << _(" or ");
        else
          first = false;

        r.get_expected(out);
      }
    };

    // The do_or routine is constructed so that the base vs non-base
    // case can be resolved as compile-time (it has to be since *iter
    // doesn't compile at all for the end iter).  The frontend routine
    // "do_or" is called as the main entry point and for recursive
    // calls, and it dispatches to "do_or_conditional" using
    // overloading and boost::fusion::equal_to to decide which case to
    // take.

    // Base case (we ran out of branches, so fail):
    template<typename ParseInput, typename CIter>
    result_type do_or_conditional(ParseInput &input, const typename ParseInput::const_iterator &initialBegin,
                                  const CIter &valuesIter, boost::mpl::true_) const
    {
      std::ostringstream msg;
      get_expected(msg);

      // ForTranslators: this is used to generate an error
      // message; a brief description of what we expected to see
      // is inserted into it.
      input.fail((boost::format(_("Expected %s")) % msg.str()).str());
    }

    // Non-base case (try the first branch):
    template<typename ParseInput, typename CIter>
    result_type do_or_conditional(ParseInput &input, const typename ParseInput::const_iterator &initialBegin,
                                  const CIter &valuesIter, boost::mpl::false_) const
    {
      try
        {
          return (*valuesIter).parse(input);
        }
      catch(ParseException &ex)
        {
          if(initialBegin != input.begin())
            throw;
        }

      // We only get here if the parse failed, so go to the next entry
      // in "values".
      return do_or(input, initialBegin, boost::fusion::next(valuesIter));
    }

    template<typename ParseInput, typename CIter>
    result_type do_or(ParseInput &input, const typename ParseInput::const_iterator &initialBegin,
                      const CIter &valuesIter) const
    {
      typedef typename boost::fusion::result_of::end<C>::type EndCIter;
      typedef typename boost::fusion::result_of::equal_to<CIter, EndCIter>::type IsAtEnd;

      return do_or_conditional(input, initialBegin, valuesIter, IsAtEnd());
    }

  public:
    or_p(const C &_values)
      : values(_values)
    {
    }

    /** \brief Retrieve the sub-parsers of this parser.
     *
     *  Mainly used by the implementation of operator|.
     */
    const C &get_values() const { return values; }

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      typename ParseInput::const_iterator initialBegin = input.begin();
      return do_or(input, initialBegin, boost::fusion::begin(values));
    }

    void get_expected(std::ostream &out) const
    {
      bool first = true;
      boost::fusion::for_each(values, do_get_expected(out, first));
    }
  };

  // Note that operator| has several overloads that use the magic of
  // Boost.Fusion to flatten expressions at compile-time.  (hopefully
  // at compile-time)

  // Note: this could be a bit more efficient if I was able to build
  // an intermediate container type that could be used only as a
  // constructor argument to "or".  That way I wouldn't need to
  // eagerly copy structures around all over.  However, doing this now
  // would be premature optimization.

  // Each of these routines creates an "or" that stores a Fusion
  // vector with copies of the input arguments.  Doing this is a bit
  // tricky since a lot of Fusion objects like to store references to
  // their input arguments.

  /** \brief Create a parser that tries the left-hand argument; if it
   *  fails without advancing "begin", then the right-hand argument is
   *  tried.
   */
  template<typename C1, typename C2>
  inline or_p<typename boost::fusion::result_of::as_vector<boost::fusion::joint_view<C1, C2> >::type>
  operator|(const or_p<C1> &o1, const or_p<C2> &o2)
  {
    // We use joint_view directly instead of via join() because
    // template parameter inference puts too many consts into the
    // template arguments otherwise.
    typedef boost::fusion::joint_view<C1, C2> interim_container;
    typedef typename boost::fusion::result_of::as_vector<interim_container>::type result_container;
    return or_p<result_container>(boost::fusion::as_vector(boost::fusion::join(o1.get_values(),
                                                                               o2.get_values())));
  }

  /** \brief Create a parser that tries the left-hand argument; if it
   *  fails without advancing "begin", then the right-hand argument is
   *  tried.
   */
  template<typename C, typename Rule, typename ResultType>
  inline or_p<typename boost::fusion::result_of::as_vector<typename boost::fusion::result_of::push_back<C, Rule>::type>::type>
  operator|(const or_p<C> &o, const parser_base<Rule, ResultType> &p)
  {
    typedef typename boost::fusion::result_of::push_back<C, Rule>::type
      interim_container;
    typedef typename boost::fusion::result_of::as_vector<interim_container>::type
      result_container;
    return or_p<result_container>(boost::fusion::as_vector(boost::fusion::push_back(o.get_values(),
                                                                                    p.derived())));
  }

  /** \brief Create a parser that tries the left-hand argument; if it
   *  fails without advancing "begin", then the right-hand argument is
   *  tried.
   */
  template<typename C, typename Rule, typename ResultType>
  inline or_p<typename boost::fusion::result_of::as_vector<boost::fusion::joint_view<boost::fusion::cons<Rule>, C> >::type>
  operator|(const parser_base<Rule, ResultType> &p, const or_p<C> &o)
  {
    typedef boost::fusion::result_of::push_front<C, Rule>
      interim_container;
    typedef typename boost::fusion::result_of::as_vector<interim_container>::type
      result_container;
    return or_p<result_container>(boost::fusion::as_vector(boost::fusion::push_front(o.get_values(),
                                                                                     p.derived())));
  }

  /** \brief Create a parser that tries the left-hand argument; if it
   *  fails without advancing "begin", then the right-hand argument is
   *  tried.
   */
  template<typename Rule1, typename ResultType1, typename Rule2, typename ResultType2>
  inline or_p<boost::fusion::vector<Rule1, Rule2> >
  operator|(const parser_base<Rule1, ResultType1> &p1, const parser_base<Rule2, ResultType2> &p2)
  {
    typedef boost::fusion::vector<Rule1, Rule2>
      result_container;

    return or_p<result_container>(result_container(p1.derived(), p2.derived()));
  }


  /** \brief A parser that attempts its sub-parser, restoring the value of
   *  "begin" if it fails.
   *
   *  Essentially makes every failure into a zero-length failure.
   */
  template<typename P>
  class maybe_p : public parser_base<maybe_p<P>, typename P::result_type>
  {
    P p;
  public:
    maybe_p(const P &_p)
      : p(_p)
    {
    }

    typedef typename P::result_type result_type;

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      ParseInput input_start(input);

      try
        {
          return p.parse(input);
        }
      catch(ParseException &ex)
        {
          // Restore the initial parse context.
          input = input_start;
          throw;
        }
    }

    void get_expected(std::ostream &out) const
    {
      p.get_expected(out);
    }
  };

  /** \brief Create a parser that attempts its parser, restoring the
   *  value of "begin" if it fails.
   */
  template<typename P>
  maybe_p<P> maybe(const P &p)
  {
    return maybe_p<P>(p);
  }

  /** \brief Apply each parser in the given Boost.Fusion container in
   *  turn, returning a tuple of their results if they all match and
   *  throwing an exception otherwise.
   *
   *  An instance of this object is constructed using operator, like
   *  so:
   *
   *  (p1 p2, p3, p4)
   */
  template<typename C>
  class tuple_p : public parser_base<tuple_p<C>,
                                     typename boost::fusion::result_of::as_vector<typename boost::mpl::transform<typename boost::fusion::result_of::as_vector<C>::type, get_result_type>::type >::type>
  // as_vector is invoked on C before we pass it to mpl::transform,
  // because mpl::transform doesn't seem to work on arbitrary fusion
  // containers (e.g., joint_view produces an error).
  {
  public:
    typedef typename boost::fusion::result_of::as_vector<typename boost::mpl::transform<typename boost::fusion::result_of::as_vector<C>::type, get_result_type>::type>::type result_type;
    typedef typename boost::fusion::result_of::as_vector<C>::type values_type;

  private:

    // Used to fold down the list of parsers and produce the output
    // list.  The state parameter will be the final return value; we
    // build it left-to-right (since fold works left-to-right).
    template<typename ParseInput>
    class do_apply
    {
      ParseInput &input;

    public:
      do_apply(ParseInput &_input)
        : input(_input)
      {
      }

      template<typename Args>
      struct result;

      template<typename Element, typename ResultIn>
      struct result<do_apply(const Element &, const ResultIn &)>
      {
        typedef typename boost::fusion::result_of::push_back<const ResultIn, typename Element::result_type>::type type;
      };

      template<typename Element, typename ResultIn>
      typename boost::fusion::result_of::push_back<const ResultIn, typename Element::result_type>::type
      operator()(const Element &e, const ResultIn &result) const
      {
        return boost::fusion::push_back(result, e.parse(input));
      }
    };

    values_type values;

  public:
    // Note that D only has to be convertible to C.  This works around
    // some places where slightly different types than we expect are
    // passed in, but was can convert them to values_type via
    // as_vector anyway.
    template<typename D>
    tuple_p(const D &_values)
      : values(boost::fusion::as_vector(_values))
    {
    }

    const values_type &get_values() const { return values; }

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      return boost::fusion::as_vector(boost::fusion::fold(values, boost::fusion::make_vector(), do_apply<ParseInput>(input)));
    }

    void get_expected(std::ostream &out) const
    {
      boost::fusion::front(values).get_expected_description(out);
    }
  };

  /** \brief Combine two tuple parsers to produce a new parser
   *  that concatenates the two tuple parsers.
   *
   *  The generated parser parses the first tuple followed by the
   *  second tuple; it returns a single tuple that contains the
   *  elements returned by the first parser, followed by the elements
   *  returned by the second parser.
   */
  template<typename C1, typename C2>
  inline tuple_p<typename boost::fusion::result_of::join<
                   typename tuple_p<C1>::values_type,
                   typename tuple_p<C2>::values_type>::type>
  operator,(const tuple_p<C1> &t1, const tuple_p<C2> &t2)
  {
    typedef typename boost::fusion::result_of::join<typename tuple_p<C1>::values_type, typename tuple_p<C2>::values_type>
      result_container;

    return tuple_p<result_container>(boost::fusion::join(t1.get_values(), t2.get_values()));
  }

  /** \brief Add a new entry to the left of a tuple parser.
   *
   *  The generated parser parses the non-tuple element followed by
   *  the elements of the tuple; it returns a single tuple that
   *  contains the element returned by the new non-tuple parser,
   *  followed by the elements returned by the tuple parser.
   */
  template<typename Rule, typename ResultType, typename C>
  inline tuple_p<typename boost::fusion::result_of::push_front<
                   typename tuple_p<C>::values_type,
                   Rule>::type>
  operator,(const parser_base<Rule, ResultType> &p1, const tuple_p<C> &t2)
  {
    typedef typename boost::fusion::result_of::push_front<typename tuple_p<C>::values_type, Rule>::type
      result_container;

    return tuple_p<result_container>(boost::fusion::push_front(t2.get_values(), p1.derived()));
  }

  /** \brief Add a new entry to the right of a tuple parser.
   *
   *  The generated parser parses the elements of the tuple, followed
   *  by the new non-tuple element; it returns a single tuple that
   *  contains the elements returned by the tuple parser, followed by
   *  the element returned by the new non-tuple parser
   */
  template<typename Rule, typename ResultType, typename C>
  inline tuple_p<typename boost::fusion::result_of::push_back<
                   typename tuple_p<C>::values_type,
                   Rule>::type>
  operator,(const tuple_p<C> &t1, const parser_base<Rule, ResultType> &p2)
  {
    typedef typename boost::fusion::result_of::push_back<typename tuple_p<C>::values_type, Rule>::type
      result_container;

    return tuple_p<result_container>(boost::fusion::push_back(t1.get_values(), p2.derived()));
  }

  /** \brief Join two parsers into a tuple parser.
   *
   *  The generated parser runs the first parser and then the second
   *  parser; it returns a tuple containing the value returned by the
   *  first parser, followed by the value returned by the second
   *  parser.
   */
  template<typename Rule1, typename ResultType1, typename Rule2, typename ResultType2>
  inline tuple_p<boost::fusion::vector<Rule1, Rule2> >
  operator,(const parser_base<Rule1, ResultType1> &p1, const parser_base<Rule2, ResultType2> &p2)
  {
    return tuple_p<boost::fusion::vector<Rule1, Rule2> >(boost::fusion::vector<Rule1, Rule2>(p1.derived(), p2.derived()));
  }

  /** \brief Create a unary tuple parser.
   *
   *  Use in conjunction with "apply" to invoke unary functions.
   *
   *  Note that since "apply" can accept a non-sequence, this is
   *  primarily useful in corner cases (e.g., if the function being
   *  applied actually takes a Fusion sequence as an argument, you
   *  need to wrap it in a tuple to avoid having the sequence become
   *  the arguments).
   */
  template<typename Rule, typename ResultType>
  inline tuple_p<boost::fusion::vector<Rule> >
  tuple(const parser_base<Rule, ResultType> &p)
  {
    return tuple_p<boost::fusion::vector<Rule> >(boost::fusion::vector<Rule>(p.derived()));
  }

  /** \brief Helper parser to apply a function object to a tuple of
   *  parsed values and return its result.
   *
   *  \tparam Func The type of function object to invoke; must be
   *               compatible with boost::fusion::fused.  In
   *               particular, this may *not* be a function type; it
   *               *must* be a class type (due to "technical reasons"
   *               aka "utter nonsense in the C++ standard").
   *
   *  \tparam P The type of the sub-parser; its return type must be a
   *            value that can be given as an argument to the result
   *            of boost::fusion::make_fused_function_object.
   */
  template<typename Func, typename P>
  class apply_p : public parser_base<apply_p<Func, P>,
                                     // The return type is the result
                                     // of applying the fused function
                                     // object to the result of the
                                     // sub-parser.
                                     typename boost::fusion::result_of::invoke<
                                       Func,
                                       typename P::result_type>::type>
  {
  public:
    typedef typename boost::fusion::result_of::invoke<Func, typename P::result_type>::type result_type;

  private:
    // It might be better to "bake in" the fusion magic by using a
    // fused_function_object here.  However, the metaprogramming got
    // too hairy when I tried going down that road.
    boost::fusion::fused<Func> func;
    P p;

  public:
    apply_p(const Func &_func, const P &_p)
      : func(_func), p(_p)
    {
    }

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      return func(p.parse(input));
    }

    void get_expected(std::ostream &out) const
    {
      p.get_expected(out);
    }
  };

  /** \brief Helper routine for apply(); do not invoke directly.
   *
   *  Handles wrapping P in a boost::fusion::vector() if it is not a
   *  Fusion sequence.
   */
  template<typename Func, typename P, typename IsSequence>
  struct internal_do_apply
  // The case where P is a Fusion sequence.
  {
    typedef apply_p<Func, P> result_type;

    result_type operator()(const Func &f, const P &p) const
    {
      return result_type(f, p);
    }
  };

  // The case where P is not a Fusion sequence.
  template<typename Func, typename P>
  struct internal_do_apply<Func, P, boost::mpl::false_>
  {
    typedef apply_p<Func, tuple_p<boost::fusion::vector<P> > > result_type;

    result_type operator()(const Func &f, const P &p) const
    {
      return result_type(f, tuple(p));
    }
  };

  /** \brief Create a helper parser to apply a function object to a
   *  tuple (or other Fusion sequence) of parsed values and return its
   *  result.
   *
   *  Mainly meant to be used with the tuple_p parser.  Also
   *  particularly useful with the construct_f function, to construct
   *  objects from parsed subexpressions.
   *
   *  \param f  The function to apply.
   *  \param p  A parser that will return either a sequence of values
   *            that can be passed to f or a single non-sequence value,
   *            which will be passed as the only argument to f.
   */
  template<typename Func, typename Rule, typename ResultType>
  inline typename internal_do_apply<
    Func, Rule,
    typename boost::fusion::traits::is_sequence<ResultType>::type>::result_type
  apply(const Func &f, const parser_base<Rule, ResultType> &p)
  {
    return internal_do_apply<
      Func, Rule,
      typename boost::fusion::traits::is_sequence<ResultType>::type>()(f, p.derived());
  }

  /** \brief Run a sub-parser, but don't advance the read position. */
  template<typename LookaheadP>
  class followedBy_p : public parser_base<followedBy_p<LookaheadP>,
                                          typename LookaheadP::result_type>
  {
  public:
    typedef typename LookaheadP::result_type result_type;

  private:
    LookaheadP lookaheadP;

  public:
    followedBy_p(const LookaheadP &_lookaheadP)
      : lookaheadP(_lookaheadP)
    {
    }

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      ParseInput lookaheadInput(input);
      return lookaheadP.parse(lookaheadInput);
    }

    void get_expected(std::ostream &out)
    {
      lookaheadP.get_expected(out);
    }
  };

  /** \brief Create a parser that runs a parser without advancing the
   *  read position.
   *
   *  Tests whether a string recognized by the parser is present at
   *  the current read position, without actually advancing the read
   *  position.  Fails if the parser fails.
   *
   *  \tparam LookaheadP The parser type used to recognize the
   *                     lookahead token.
   *
   *  \param lookahead   The parser used to recognize the
   *                     lookahead token.
   */
  template<typename LookaheadP>
  followedBy_p<LookaheadP> followedBy(const LookaheadP &lookaheadP)
  {
    return followedBy_p<LookaheadP>(lookaheadP);
  }

  /** \brief Run a sub-parser, but don't advance the read position;
   *  succeed only if the sub-parser fails.
   */
  template<typename LookaheadP>
  class notFollowedBy_p : public parser_base<notFollowedBy_p<LookaheadP>,
                                             nil_t>
  {
  public:
    typedef nil_t result_type;

  private:
    LookaheadP lookaheadP;

  public:
    notFollowedBy_p(const LookaheadP &_lookaheadP)
      : lookaheadP(_lookaheadP)
    {
    }

    template<typename ParseInput>
    result_type apply(ParseInput &input) const
    {
      try
        {
          ParseInput lookaheadInput = input;
          lookaheadP.parse(lookaheadInput);
        }
      catch(ParseException &)
        {
          return nil_t();
        }

      std::ostringstream msg;
      lookaheadP.get_expected_description(msg);
      input.fail((boost::format(_("Unexpected %s")) % msg.str()).str());
    }

    void get_expected(std::ostream &out)
    {
      out << "not ";
      lookaheadP.get_expected(out);
    }
  };

  /** \brief Create a parser that succeeds only if the given parser
   *         fails.
   *
   *  Tests whether a string recognized by the parser is present at
   *  the current read position, without actually advancing the read
   *  position.  Fails if the parser succeeds.
   *
   *  \tparam LookaheadP The parser type used to recognize the
   *                     lookahead token.
   *
   *  \param lookahead   The parser used to recognize the
   *                     lookahead token.
   */
  template<typename LookaheadP>
  notFollowedBy_p<LookaheadP> notFollowedBy(const LookaheadP &lookaheadP)
  {
    return notFollowedBy_p<LookaheadP>(lookaheadP);
  }

  /** Used to generate operator() overloads for the type C in the
   *  construct_f class.
   */
#define PARSERS_MAKE_CONSTRUCTOR_WRAPPER( z, n, C )                     \
  template<BOOST_PP_ENUM_PARAMS(n, typename Arg)>                       \
  C operator()(BOOST_PP_ENUM_BINARY_PARAMS(n, const Arg, &arg)) const   \
  {                                                                     \
    return C(BOOST_PP_ENUM_PARAMS(n, arg));                             \
  }

  /** \brief The maximum number of parameters that can be passed to a
   *  constructor wrapped by construct_f.
   */
#define MAX_CONSTRUCTOR_F_ARGS 20

  /** \brief A function object whose operator() invokes a constructor
   *  on the given type.
   *
   *  \tparam C The type to construct.
   */
  template<typename C>
  struct construct_f
  {
    typedef C result_type;

    // The nullary constructor is handled specially since it shouldn't
    // have a template parameter list.
    C operator()() const
    {
      return C();
    }

    BOOST_PP_REPEAT_FROM_TO( 1, MAX_CONSTRUCTOR_F_ARGS, PARSERS_MAKE_CONSTRUCTOR_WRAPPER, C );
  };

#undef PARSERS_MAKE_CONSTRUCTOR_WRAPPER

  /** \brief Convenience parsers
   *
   *  These parsers are defined in terms of other parsers; they aren't
   *  strictly necessary in the library, but they're often useful and
   *  having them available saves lots of typing.
   */
  // @{

  /** \brief Function object to test whether a shared pointer to an
   *  STL-style sequence is not empty.
   *
   *  Used to implement manyOne.
   */
  template<typename Seq>
  struct notEmpty_f
  {
    bool operator()(const boost::shared_ptr<Seq> &s) const
    {
      return !s->empty();
    }
  };

  /** \brief Metafunction computing the type of parser returned by
   *  manyOne.
   */
  template<typename P, typename Seq>
  class manyOne_result
  {
  public:
    typedef assert_p<many_p<P, Seq>, notEmpty_f<Seq> > type;
  };

  // Sanity-check that we get the right type out of a manyOne
  // expression.
  BOOST_STATIC_ASSERT( (boost::is_same< assert_p<many_p<charif_p<char, digit_f>, std::string>, notEmpty_f<std::string> >,
                                        manyOne_result<charif_p<char, digit_f>, std::string>::type>::value) );
  BOOST_STATIC_ASSERT( (boost::is_same< boost::shared_ptr<std::string>,
                                        manyOne_result<charif_p<char, digit_f>, std::string>::type::result_type>::value) );

  /** \brief Match one or more copies of the given parser.
   *
   *  \tparam P   The type of parser to match.
   *  \tparam Seq The STL sequence type to return.
   *
   *  \param p  The parser to match.
   */
  template<typename P, typename Seq>
  typename manyOne_result<P, Seq>::type
  manyOne_p(const P &p)
  {
    std::ostringstream msg;
    p.get_expected_description(msg);

    return postAssert(many_p<P, Seq>(p),
                      msg.str(),
                      notEmpty_f<Seq>());
  }

  /** \brief Match one or more copies of a parser and build a vector from them.
   *
   *  \tparam P  The type of parser to match.
   *  \param p   The parser to match.
   */
  template<typename P>
  typename manyOne_result<P, std::vector<typename P::result_type> >::type
  manyOne(const P &p)
  {
    return manyOne_p<P, std::vector<typename P::result_type> >(p);
  }

  /** \brief Match one or more copies of a parser and build a string from them.
   *
   *  \tparam P  The type of parser to match.
   *  \param p   The parser to match.
   */
  template<typename P>
  typename manyOne_result<P, std::string>::type
  manyOne_string(const P &p)
  {
    return manyOne_p<P, std::string>(p);
  }

  /** \brief Metafunction computing the result type of optional(). */
  template<typename P>
  struct optional_result
  {
    typedef boost::optional<typename P::result_type> parse_type;
    typedef or_p<boost::fusion::vector<apply_p<construct_f<parse_type>, tuple_p<boost::fusion::vector<P> > >,
                                       val_p<parse_type> > >
    type;
  };

  /** \brief Match zero or one copies of the given parser.
   *
   *  There are three ways this can behave:
   *
   *   1. If the parser succeeds, its value is returned.
   *   2. If the parser fails without consuming input, an empty optional
   *      value is returned.
   *   3. If the parser fails and consumes input, this parser fails.
   */
  template<typename P>
  typename optional_result<P>::type optional(const P &p)
  {
    return
        apply(construct_f<boost::optional<typename P::result_type> >(), tuple(p))
      | val(boost::optional<typename P::result_type>());
  }

  // @}
}

#endif // PARSERS_H
