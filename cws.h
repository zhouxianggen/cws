/*
 * chinese word segment 
 * Copyright (C) 2012 UC
 * Author: zhouxg@ucweb.com
 *
 */
#ifndef	UCLTP_CWS_H_
#define	UCLTP_CWS_H_
#include <vector>
#include <string>
using namespace std;
#include "common.h"
namespace ucltp {

typedef  vector<char>          Tags;
typedef  vector<Char>          Chars;
typedef  vector<string>        Words;

class ChineseWordSegment {
public:
	enum Flags { NONE=0, INGOR_STOP_WORD=0x1 };
    bool init(const char *cws_data_dir);
    bool seg(const char *text, Words &words, int flag=NONE);
    ChineseWordSegment() 
        : cfg_(0), dict_(0), tagger_(0), swr_(0) {}
    ~ChineseWordSegment ();
private:
	bool init();
    void release();
    bool tokenize(const char *text, Chars &chars);
    bool rule_seg(const Chars &chars, Tags &tags);
    bool dict_seg(const Chars &chars, Tags &tags);
    bool stat_seg(const Chars &chars, Tags &tags);
    bool get_context(const Chars &chars, int i, char *context, int len);
    void get_words(const Chars &chars, const Tags &tags, Words &words, int flag);
    void *cfg_;
    void *dict_;
    void *tagger_;
	void *swr_;
};

}
#endif
