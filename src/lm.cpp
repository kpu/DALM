#include <cmath>

#include <stdint.h>
#include <unistd.h>
#include <vector>

#include "dalm.h"
#include "arpafile.h"
#include "version.h"
#include "pthread_wrapper.h"
#include "handler.h"

using namespace DALM;

LM::LM(std::string pathtoarpa, std::string pathtotree, Vocabulary &vocab, size_t dividenum, unsigned int opt, Logger &logger)
		:vocab(vocab), v(Version(opt, logger)), logger(logger) {
	logger << "[LM::LM] TEXTMODE begin." << Logger::endi;
	logger << "[LM::LM] pathtoarpa=" << pathtoarpa << Logger::endi;
	logger << "[LM::LM] pathtotree=" << pathtotree << Logger::endi;

	build(pathtoarpa, pathtotree, dividenum);
	std::string stagstart = "<s>";
	stagid = vocab.lookup(stagstart.c_str());
	logger << "[LM::LM] end." << Logger::endi;
}

LM::LM(std::string dumpfilepath,Vocabulary &vocab, unsigned char order, Logger &logger)
	:vocab(vocab), logger(logger){

	logger << "[LM::LM] BINMODE begin." << Logger::endi;
	logger << "[LM::LM] dumpfilepath=" << dumpfilepath << Logger::endi;

	FILE *fp = fopen(dumpfilepath.c_str(), "rb");
	if(fp != NULL){
		v = Version(fp, logger); // Version check.
		readParams(fp, order);
		fclose(fp);
	} else {
		logger << "[LM::LM] Dump file may have IO error." << Logger::endc;
		throw "error.";
	}
	std::string stagstart = "<s>";
	stagid = vocab.lookup(stagstart.c_str());
	logger << "[LM::LM] end." << Logger::endi;
}

LM::~LM() {
	delete handler;
}

float LM::query(VocabId *ngram, size_t n){
	return handler->get_prob(ngram, n);
}

float LM::query(VocabId word, State &state){
	return handler->get_prob(word, state);
}

float LM::query(VocabId word, State &state, Fragment &f){
	return handler->get_prob(word, state, f);
}

float LM::query(const Fragment &f, State &state_prev, Gap &gap){
	return handler->get_prob(f, state_prev, gap);
}

float LM::query(const Fragment &fprev, State &state_prev, Gap &gap, Fragment &fnew){
	return handler->get_prob(fprev, state_prev, gap, fnew);
}

void LM::init_state(State &state){
	handler->init_state(&stagid, 1, state);
}

void LM::set_state(VocabId *ngram, size_t n, State &state){
	handler->init_state(ngram, n, state);
}

void LM::set_state(State &state, const State &state_prev, const Fragment *fragments, const Gap &gap){
	handler->init_state(state, state_prev, fragments, gap);
}

/* depricated */
StateId LM::get_state(VocabId *ngram, size_t n){
	for(size_t i=0;i<n;i++){
		if(ngram[i] == vocab.unk()){
			n=i;
			break;
		}
	}
	return handler->get_state(ngram, n);
}

void LM::errorcheck(std::string &pathtoarpa){
  ARPAFile *arpafile = new ARPAFile(pathtoarpa, vocab);
  logger << "[LM::errorcheck] start." << Logger::endi;
  int error_num = 0;
  size_t total = arpafile->get_totalsize();
  for(size_t i=0;i<total;i++){
    unsigned short n;
    VocabId *ngram;
    float prob;
    float bow;
    bool bow_presence = arpafile->get_ngram(n,ngram,prob,bow);
    
    bool builderror = handler->errorcheck(n, ngram, bow, prob, bow_presence);
    if(!builderror){
      logger << "[LM::build] Error: line=" << i << Logger::ende;
      error_num++;
    }
    delete [] ngram;
  }
  delete arpafile;
  logger << "[LM::build] # of errors: " << error_num << Logger::endi;
}

void LM::build(std::string &pathtoarpa, std::string &pathtotreefile, size_t dividenum){
	logger << "[LM::build] LM build begin." << Logger::endi;

	if(v.get_opt()==DALM_OPT_EMBEDDING){
		handler = new EmbeddedDAHandler();
	}else{
		logger << "[LM::build] DALM unknown type." << Logger::endc;
		throw "type error";
	}
	handler->build(dividenum, pathtoarpa, pathtotreefile, vocab, logger);

	logger << "[LM::build] DoubleArray build end." << Logger::endi; 	
	errorcheck(pathtoarpa);

	logger << "[LM::build] LM build end." << Logger::endi;
}

void LM::dumpParams(FILE *fp){
	handler->dump(fp);
}

void LM::readParams(FILE *fp, unsigned char order){
	if(v.get_opt()==DALM_OPT_EMBEDDING){
		handler = new EmbeddedDAHandler(fp, order, logger);
	}else{
		logger << "[LM::build] DALM unknown type." << Logger::endc;
		throw "type error";
	}
}

