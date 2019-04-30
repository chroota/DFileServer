#include "logger.hpp"


bool Logger::saveLog(const string &info){
    if(LOG_SAVE){
        //todo save
    }
    return false;
}



string Logger::getTimeFmtString()
{
    char buf[MAX_LOG_INFO_BUF_SIZE];
    time_t now = time(nullptr);
    struct tm stime;
    localtime_r(&now, &stime);
	strftime(buf, sizeof(buf), "[%F %T] ", &stime);
    buf[strlen(buf)] = 0;
    return string(buf);
}

void Logger::log(verbosity verbose, const char* fmt, ...)
{

	char buf[MAX_LOG_INFO_BUF_SIZE];
	size_t bpos;

    ostringstream oss;
    oss<<getTimeFmtString();

	va_list args;
	va_start(args, fmt);
	vsnprintf(buf, sizeof(buf), fmt, args);
	va_end(args);

    oss<<buf;
    if(LOG_DEBUG && verbose == LDEBUG){
       oss<<"(DEBUG)";
    }

    if(verbose == L4){
        oss<<"(FATAL)";
    }
    oss<<endl;
    cout<<oss.str();
    saveLog(oss.str());
    if(verbose == L4){
        exit(-1);
    }
}

void Logger::log(string info, verbosity level)
{
    log(level, info.c_str());
}

void Logger::debug(const string &info)
{
    log(LDEBUG, info.c_str());
}

void Logger::debugAction(const string &action)
{
    ostringstream oss;
    oss << "action:" << action;
    log(oss.str());
}

void Logger::fatal(const string &info){
    log(L4, info.c_str());
}

void Logger::errnoInfo(int errno){
    ostringstream oss;
    oss << "errno:"<<errno<<" info:"<<strerror(errno);
    log(oss.str());
}