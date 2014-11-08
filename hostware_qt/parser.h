/** \file parser.h
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


#ifndef PARSER_H_
#define PARSER_H_

#include <QObject>


enum tokenizerState {
  TOKENIZER_START,
  TOKENIZER_GET_TIMERCOUNTS,
  TOKENIZER_GET_KERNELTIME,
  TOKENIZER_GET_ACCUCOUNTS
};


/** payload data which is sent over the character device */
class payloadData
{
  public:
    int timerCounts;
    int kernelTime;
    int accuCounts;
  private:
};


/* we need the QObject to implement signals and slots */
class Parser : public QObject
{
    Q_OBJECT

public:
    explicit Parser ();
    ~Parser ();
    int doParse(const char *stream, int len);

signals:
   void parserDataReady(const payloadData *data);

public slots:


private:
   int screener(const char *stream, int len);
   int isASeparator(char ch);
   int tokenizer(const char *token, int len);
   payloadData mPayloadData;
   tokenizerState mTokenizerState = TOKENIZER_START;
};

#endif
