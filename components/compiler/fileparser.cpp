#include "fileparser.hpp"

#include <iostream>

#include "tokenloc.hpp"
#include "scanner.hpp"

namespace Compiler
{
    FileParser::FileParser (ErrorHandler& errorHandler, Context& context)
    : Parser (errorHandler, context),
      mScriptParser (errorHandler, context, mLocals, true),
      mState (BeginState)
    {}

    std::string FileParser::getName() const
    {
        return mName;
    }

    void FileParser::getCode (std::vector<Interpreter::Type_Code>& code) const
    {
        mScriptParser.getCode (code);
    }

    const Locals& FileParser::getLocals() const
    {
        return mLocals;
    }

    bool FileParser::parseName (const std::string& name, const TokenLoc& loc,
        Scanner& scanner)
    {
        if (mState==NameState)
        {
            mName = name;
            mState = BeginCompleteState;
            return true;
        }

        if (mState==EndNameState)
        {
            // optional repeated name after end statement
            if (mName!=name)
                reportWarning ("Names for script " + mName + " do not match", loc);

            mState = EndCompleteState;
            return false; // we are stopping here, because there might be more garbage on the end line,
                          // that we must ignore.
                          //
                          /// \todo allow this workaround to be disabled for newer scripts
        }

        if (mState==BeginCompleteState)
        {
            reportWarning ("Stray string (" + name + ") after begin statement", loc);
            return true;
        }

        return Parser::parseName (name, loc, scanner);
    }

    bool FileParser::parseKeyword (int keyword, const TokenLoc& loc, Scanner& scanner)
    {
        if (mState==BeginState && keyword==Scanner::K_begin)
        {
            mState = NameState;
            scanner.allowNameStartingwithDigit();
            return true;
        }

        if (mState==NameState)
        {
            // keywords can be used as script names too. Thank you Morrowind for another
            // syntactic perversity :(
            mName = loc.mLiteral;
            mState = BeginCompleteState;
            return true;
        }

        if (mState==EndNameState)
        {
            // optional repeated name after end statement
            if (mName!=loc.mLiteral)
                reportWarning ("Names for script " + mName + " do not match", loc);

            mState = EndCompleteState;
            return false; // we are stopping here, because there might be more garbage on the end line,
                          // that we must ignore.
                          //
                          /// \todo allow this workaround to be disabled for newer scripts
        }

        return Parser::parseKeyword (keyword, loc, scanner);
    }

    bool FileParser::parseSpecial (int code, const TokenLoc& loc, Scanner& scanner)
    {
        if (code==Scanner::S_newline)
        {
            if (mState==BeginState)
            {
                // ignore empty lines
                return true;
            }

            if (mState==BeginCompleteState)
            {
                // parse the script body
                mScriptParser.reset();

                scanner.scan (mScriptParser);

                mState = EndNameState;
                scanner.allowNameStartingwithDigit();
                return true;
            }

            if (mState==EndCompleteState || mState==EndNameState)
            {
                // we are done here -> ignore the rest of the script
                return false;
            }
        }

        return Parser::parseSpecial (code, loc, scanner);
    }

    void FileParser::parseEOF (Scanner& scanner)
    {
        if (mState!=EndNameState && mState!=EndCompleteState)
            Parser::parseEOF (scanner);
    }

    void FileParser::reset()
    {
        mState = BeginState;
        mName.clear();
        mScriptParser.reset();
        Parser::reset();
    }
}
