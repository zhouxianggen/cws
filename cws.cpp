
#include <stdio.h>
#include <math.h>
#include "crfpp.h"
using namespace CRFPP;
#include "config.h"
#include "swr.h"
#include "cws.h"
namespace ucltp {

ChineseWordSegment::~ChineseWordSegment ()
{
    release();
}

void ChineseWordSegment::release()
{
    if (tagger_) delete (Tagger*)tagger_;
	if (cfg_) delete (Config*)cfg_;
	if (swr_) delete (StopWordRecognizer*)swr_;
}

bool ChineseWordSegment::init(const char *cws_data_dir)
{
    release();

    // new config
    Config *cfg = new Config;
    cfg_ = (Config*)cfg;
    if (!cfg)
        return false;

    cfg->set("cws", "data_dir", cws_data_dir);
    
    return init();
}
/*
bool ChineseWordSegment::init(const char *config)
{
    release();

    // read config
    Config *cfg = new Config;
    cfg_ = (Config*)cfg;
    if (!cfg || !cfg->parse(config))
        return false;
    
    return init();
}
*/
bool ChineseWordSegment::init()
{
    Config *cfg = (Config*)cfg_;
    if (!cfg) return false;

    // init crf
    char arg[256];
    sprintf(arg, "-m %s/cws.crf.model -v3", cfg->get("cws", "data_dir").c_str());
    Tagger *tagger = createTagger(arg);
    tagger_ = (void*)tagger;
    if (!tagger)
        return false;
	// init swr
	sprintf(arg, "%s/stopwords", cfg->get("cws", "data_dir").c_str());
	StopWordRecognizer *swr = new StopWordRecognizer();
	swr_ = (void*)swr;
	if (!swr || swr->init(arg)<0)
		return false;

    return true;
}

void ChineseWordSegment::get_words(const Chars &chars, const Tags &tags, Words &words, int flag)
{
	StopWordRecognizer *swr = (StopWordRecognizer*)swr_;
    words.clear();
    for (int i=0,s=chars.size(); i<s; ) {
		string w(chars[i].name_);
		for (i=i+1; i<s && tags[i]!='S' && tags[i]!='B'; i+=1)
			w += string(chars[i].name_);
		if ((flag & INGOR_STOP_WORD) && swr && swr->check(w.c_str()))
			continue;
		words.push_back(w);
    }
}

bool ChineseWordSegment::tokenize(const char *text, Chars &chars)
{
    uint32 len, code, i;
    if (!text) return false;
    for (i=0; getu8char(text+i, len, code); i+=len) {
        code = u2l(d2s(code));
        if (code > 0xFFFF) return false;
		if (code <= 0x7F) chars.push_back(Char(char(code), i, code)); 
		else chars.push_back(Char(text+i, len, i, code));
    }
    return !text[i];
}

bool ChineseWordSegment::seg(const char *text, Words &words, int flag)
{
    Chars chars;
    if (!tokenize(text, chars))
        return false;
   	
    Tags tags(chars.size(), 'B');
    stat_seg(chars, tags);
	//for (int i=0; i<tags.size(); i+=1) printf("%s/%c  ", chars[i].name_, tags[i]); printf("\n");
    rule_seg(chars, tags);
	//for (int i=0; i<tags.size(); i+=1) printf("%s/%c  ", chars[i].name_, tags[i]); printf("\n");
    get_words(chars, tags, words, flag);
    
	return true;
}

bool ChineseWordSegment::rule_seg(const Chars &chars, Tags &tags)
{
    for (int i=0,s=chars.size(); i<s; i+=1) {
		uint16 u = chars[i].code_;
		if (isuchn(u)) continue;
		else if (isualnum(u)) {
			if (i>0 && isualnum(chars[i-1].code_)) tags[i] = 'I';
			if (i+1<s && !isualnum(chars[i+1].code_)) tags[i+1] = 'B';
		}
		else {
			tags[i] = 'B';
			if (i+1 < s) tags[i+1] = 'B';
		}
    }
	return true;
}

bool ChineseWordSegment::dict_seg(const Chars &chars, Tags &tags)
{
}

bool ChineseWordSegment::get_context(const Chars &chars, int i, char *context, int len)
{
	int j = 0;
	if (!context || len<=0) return false;
	if (chars[i].code_ <= 0x7F)
		context[j++] = char(chars[i].code_);
	else {
		for (int k=chars[i].end_-chars[i].beg_; j<k && j<len; j+=1)
			context[j] = chars[i].name_[j];
	}
	if (j >= len) return false;
	context[j] = '\0';
	return true;
}

bool ChineseWordSegment::stat_seg(const Chars &chars, Tags &tags)
{
	if (!tagger_) return false;
    Tagger *tagger = (Tagger*)tagger_;
	char context[512];
    for (int i=0,s=chars.size(); i<=s; i+=1) {
        if (i<s && !isuspace(chars[i].code_)) {
            if (!get_context(chars, i, context, 512)) return false;
            tagger->add(context);
        }
        else {
			tagger->parse();
			for (int j=0,ts=tagger->size(); j<ts; j+=1)
				tags[i-ts+j] = tagger->y2(j)[0];
			//for (int j=0,ts=tagger->size(); j<ts; j+=1) printf("%s/%s  ", chars[i-ts+j].name_, tagger->y2(j)); printf("\n");
            if (i < s) tags[i] = 'B';
            tagger->clear();
        }
    }
}

} // namepsace

