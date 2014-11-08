/** \file parser.cpp
* \brief Parser to convert the kernel device character stream into numbers
*
* \author Copyright (C) 2014 samplemaker
*
* This library is free software; you can redistribute it and/or
* modify it under the terms of the GNU Lesser General Public License
* as published by the Free Software Foundation; either version 2.1
* of the License, or (at your option) any later version.
*
* This library is distributed in the hope that it will be useful,
* but WITHOUT ANY WARRANTY; without even the implied warranty of
* MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
* Lesser General Public License for more details.
*
* You should have received a copy of the GNU Lesser General Public
* License along with this library; if not, write to the Free
* Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
* Boston, MA 02110-1301 USA
*
* @{
*/


#include <QtCore/QDebug>
#include "parser.h"


Parser::Parser() {

}


Parser::~Parser()
{

}


/** Parser entry function */
int
Parser::doParse(const char *stream, int len){
  return (screener(stream, len));
}


/** Handle separation characters. "\n" is handled as a token */
int
Parser::isASeparator(char ch){
  return ((ch == '\t') | (ch == ' ') | (ch == ';'));
}


/** Removes separators and converts the character stream into a character token stream */
int
Parser::screener(const char *stream, int len){
  static int j = 0;
  static char token[1024];
  int err = 0;
 
  //qWarning() << "lexer input:" << stream;

  for (int i = 0; i < len; i++){
     if ( (!isASeparator(stream[i])) && stream[i] != '\n'){
       /* must be start of a token - collect the token data */
       token[j] = stream[i];
       j++;
     }else{
       if ( stream[i] == '\n' ){
         /* if there was no regular token before the '\n' separator ('\n' at the start of
            the line or just before '\n' is a separator) then j == 0 and the tokenizer call
            will return without any action */
         token[j] = '\0';
         if (tokenizer(token, j) < 0) 
           err = -1;
         j = 0;
         /* inform the tokenizer about the '\n' new line (special token) to reset its
            internal state machine (necessary if lets say a broken line is parsed) */
         if (tokenizer(&stream[i], 1) < 0)
           err = -1;
       }else{
         /* a regular token is completed and can be sent to the tokenizer or a separator
            is found but then j==0 and a tokanizer call will return without any action */
         token[j] = '\0';
         if (tokenizer(token, j) < 0)
           err = -1;
         j = 0;
       }
     }
  }
  return err;
}


/** Identify, interpret and convert the tokens into the different data types (lexer) */
int
Parser::tokenizer(const char *token, int len){
  char * pEnd;
  //qWarning() << "tokenizer input:" << token << "len:" << len;

  if (len > 0){
    /* check for end of line token '\n'. if there is a '\n' somewhere in the stream it is the
       lexers responsibility to seperate the '\n'-token from the stream. we dont need
       plausibility checks for '\n'-tokens within regular tokens */
    if (token[0] != '\n'){
      /* regular token (data-token or comment-token). parse the token */
      switch (mTokenizerState){
         case TOKENIZER_START:
             /* presume start of a line. first token is a comment, can be everything */
             mTokenizerState = TOKENIZER_GET_TIMERCOUNTS;
         break;
         case TOKENIZER_GET_TIMERCOUNTS:
             mPayloadData.timerCounts = strtol(token, &pEnd, 10);
             if ((pEnd - token) != len){
               /* cant convert, ERROR_SYNOPSIS */
               mTokenizerState = TOKENIZER_START;
               return -1;
             }else{
               mTokenizerState = TOKENIZER_GET_KERNELTIME;
             }
         break;
         case TOKENIZER_GET_KERNELTIME:
             mPayloadData.kernelTime = strtol(token, &pEnd, 10);
             if ((pEnd - token) != len){
               /* cant convert, ERROR_SYNOPSIS */
               mTokenizerState = TOKENIZER_START;
               return -1;
             }else{
               mTokenizerState = TOKENIZER_GET_ACCUCOUNTS;
             }
         break;
         case TOKENIZER_GET_ACCUCOUNTS:
             mPayloadData.accuCounts = strtol(token, &pEnd, 10);
             if ((pEnd - token) != len){
               /* cant convert, ERROR_SYNOPSIS */
               mTokenizerState = TOKENIZER_START;
               return -1;
             }else{
                emit parserDataReady(&mPayloadData);
                /* qWarning() << "CNT:" << mPayloadData.timerCounts
                           << "TIME:" << mPayloadData.kernelTime
                           << "ACCU:" << mPayloadData.accuCounts; */
             }
         break;
         default:
         break;
      }
    }else{
      /* '\n'-token found, start over with new line, reset state machine */
      tokenizerState stateOld = mTokenizerState;
      mTokenizerState = TOKENIZER_START;
      /* unplausible end of line */
      if (!((stateOld == TOKENIZER_START) | (stateOld == TOKENIZER_GET_ACCUCOUNTS)))
        return -1;
    }
  }
  return 0;
}
