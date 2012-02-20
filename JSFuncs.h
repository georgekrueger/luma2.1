#ifndef __JSFUNC_H__
#define __JSFUNC_H__

#include <v8.h>

bool ExecuteString(v8::Handle<v8::String> source,
                   v8::Handle<v8::Value> name,
                   bool print_result,
                   bool report_exceptions);
v8::Handle<v8::Value> Print(const v8::Arguments& args);
v8::Handle<v8::String> ReadFile(const char* name);
void ReportException(v8::TryCatch* handler);
const char* ToCString(const v8::String::Utf8Value& value);

#endif