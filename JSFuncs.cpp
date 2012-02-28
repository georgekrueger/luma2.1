
#include "JSFuncs.h"
#include "music.h"
#include <assert.h>
#include <iostream>
#include <sstream>
#include <boost/variant.hpp>

using namespace v8;

// Note
static Persistent<ObjectTemplate> gNoteTemplate;
static Persistent<ObjectTemplate> gRestTemplate;
static Persistent<ObjectTemplate> gPatternTemplate;
v8::Handle<v8::Value> MakeNote(const v8::Arguments& args);
Handle<ObjectTemplate> MakeNoteTemplate();
v8::Handle<v8::Value> MakeRest(const v8::Arguments& args);
Handle<ObjectTemplate> MakeRestTemplate();
v8::Handle<v8::Value> MakePattern(const v8::Arguments& args);
Handle<ObjectTemplate> MakePatternTemplate();
Handle<Value> GetPitch(Local<String> name, const AccessorInfo& info);

typedef boost::variant<WeightedEvent*, Pattern*> JSObjectHolder;

Persistent<Context> CreateV8Context()
{
	v8::HandleScope handle_scope;

	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();

	// Bind the global 'print' function to the C++ Print callback.
	global->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));
	global->Set(v8::String::New("note"), v8::FunctionTemplate::New(MakeNote));
	global->Set(v8::String::New("rest"), v8::FunctionTemplate::New(MakeRest));
	global->Set(v8::String::New("pattern"), v8::FunctionTemplate::New(MakePattern));

	v8::Persistent<v8::Context> context = v8::Context::New(NULL, global);

	return context;
}

/**
 * Utility function that extracts the C++ object from a JS wrapper object.
 */
template <class T>
T* ExtractObjectFromJSWrapper(Handle<Object> obj) {
	Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
	void* ptr = field->Value();
	return static_cast<T*>(ptr);
}

// The callback that is invoked by v8 whenever the JavaScript 'print'
// function is called.  Prints its arguments on stdout separated by
// spaces and ending with a newline.
v8::Handle<v8::Value> Print(const v8::Arguments& args) {
  bool first = true;
  for (int i = 0; i < args.Length(); i++) {
    v8::HandleScope handle_scope;
    if (first) {
      first = false;
    } else {
      printf(" ");
    }
    v8::String::Utf8Value str(args[i]);
    const char* cstr = ToCString(str);
    printf("%s", cstr);
  }
  printf("\n");
  fflush(stdout);
  return v8::Undefined();
}

// Executes a string within the current v8 context.
bool ExecuteString(v8::Handle<v8::String> source,
                   v8::Handle<v8::Value> name,
                   bool print_result,
                   bool report_exceptions) {
  v8::HandleScope handle_scope;
  v8::TryCatch try_catch;
  v8::Handle<v8::Script> script = v8::Script::Compile(source, name);
  if (script.IsEmpty()) {
    // Print errors that happened during compilation.
    if (report_exceptions)
      ReportException(&try_catch);
    return false;
  } else {
    v8::Handle<v8::Value> result = script->Run();
    if (result.IsEmpty()) {
      assert(try_catch.HasCaught());
      // Print errors that happened during execution.
      if (report_exceptions)
        ReportException(&try_catch);
      return false;
    } else {
      assert(!try_catch.HasCaught());
      if (print_result && !result->IsUndefined()) {
        // If all went well and the result wasn't undefined then print
        // the returned value.
        v8::String::Utf8Value str(result);
        const char* cstr = ToCString(str);
        printf("%s\n", cstr);
      }
      return true;
    }
  }
}

// Reads a file into a v8 string.
v8::Handle<v8::String> ReadFile(const char* name) {
  FILE* file = fopen(name, "rb");
  if (file == NULL) return v8::Handle<v8::String>();

  fseek(file, 0, SEEK_END);
  int size = ftell(file);
  rewind(file);

  char* chars = new char[size + 1];
  chars[size] = '\0';
  for (int i = 0; i < size;) {
    int read = fread(&chars[i], 1, size - i, file);
    i += read;
  }
  fclose(file);
  v8::Handle<v8::String> result = v8::String::New(chars, size);
  delete[] chars;
  return result;
}

// Extracts a C string from a V8 Utf8Value.
const char* ToCString(const v8::String::Utf8Value& value) {
  return *value ? *value : "<string conversion failed>";
}

void ReportException(v8::TryCatch* try_catch) {
  v8::HandleScope handle_scope;
  v8::String::Utf8Value exception(try_catch->Exception());
  const char* exception_string = ToCString(exception);
  v8::Handle<v8::Message> message = try_catch->Message();
  if (message.IsEmpty()) {
    // V8 didn't provide any extra information about this error; just
    // print the exception.
    printf("%s\n", exception_string);
  } else {
    // Print (filename):(line number): (message).
    v8::String::Utf8Value filename(message->GetScriptResourceName());
    const char* filename_string = ToCString(filename);
    int linenum = message->GetLineNumber();
    printf("%s:%i: %s\n", filename_string, linenum, exception_string);
    // Print line of source code.
    v8::String::Utf8Value sourceline(message->GetSourceLine());
    const char* sourceline_string = ToCString(sourceline);
    printf("%s\n", sourceline_string);
    // Print wavy underline (GetUnderline is deprecated).
    int start = message->GetStartColumn();
    for (int i = 0; i < start; i++) {
      printf(" ");
    }
    int end = message->GetEndColumn();
    for (int i = start; i < end; i++) {
      printf("^");
    }
    printf("\n");
    v8::String::Utf8Value stack_trace(try_catch->StackTrace());
    if (stack_trace.length() > 0) {
      const char* stack_trace_string = ToCString(stack_trace);
      printf("%s\n", stack_trace_string);
    }
  }
}

unsigned long WEIGHT_SCALE = 1000;

void ExtractPitch(Handle<Value> val, Scale& scale, short& octave, short& degree)
{
	scale = NO_SCALE;
	octave = 1;
	degree = 1;

	v8::String::Utf8Value str(val);
	string pitchStr = string(ToCString(str));
	size_t firstSplit = pitchStr.find_first_of('_');
	size_t secondSplit = pitchStr.find_last_of('_');
	string scaleStr = pitchStr.substr(0, firstSplit);
	string octaveStr = pitchStr.substr(firstSplit+1, 1);
	string degreeStr = pitchStr.substr(secondSplit+1, 1);
	cout << scaleStr << " " << octaveStr << " " << degreeStr << endl;
	for (int i=0; i<NumScales; i++) {
		if (scaleStr.compare(ScaleStrings[i]) == 0) {
			scale = (Scale)i;
			break;
		}
	}
	if (scale == NO_SCALE) {
		cout << "Invalid scale: " << scaleStr << endl;
		scale = CMAJ;
	}
	stringstream octaveStream(octaveStr);
	octaveStream >> octave;
	stringstream degreeStream(degreeStr);
	degreeStream >> degree;
}

Handle<Value> MakeNote(const Arguments& args) {
	HandleScope handle_scope;

	if (args.Length() != 3) {
		cout << "Error creating a note: " << endl;
	}
	
	WeightedEvent* noteEvent = new WeightedEvent;
	noteEvent->type = NOTE_ON;

	Local<Value> arg;

	arg = args[0];
	if (arg->IsArray()) {
		Array* arr = Array::Cast(*arg);
		for (int i=0; i<arr->Length(); i++) {
			Local<Value> elem = arr->Get(i);
			if (elem->IsArray()) {
				// weighted length
				Array* pair = Array::Cast(*elem);

				Scale scale;
				short octave;
				short degree;
				ExtractPitch(pair->Get(0), scale, octave, degree);
				cout << "Scale: " << scale << " octave: " << octave << " degree: " << degree << endl;

				double weight = pair->Get(1)->NumberValue();
				noteEvent->pitch.push_back(GetMidiPitch(scale, octave, degree));
				noteEvent->pitchWeight.push_back(static_cast<unsigned long>(weight * WEIGHT_SCALE));
			}
			else if (elem->IsNumber()) {
				
				Scale scale;
				short octave;
				short degree;
				ExtractPitch(elem, scale, octave, degree);
				cout << "Scale: " << scale << " octave: " << octave << " degree: " << degree << endl;
				
				noteEvent->pitch.push_back(GetMidiPitch(scale, octave, degree));
				noteEvent->pitchWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
			}
			else {
				// error
			}
		}
	}
	else {
		Scale scale;
		short octave;
		short degree;
		ExtractPitch(arg, scale, octave, degree);
		cout << "Scale: " << scale << " octave: " << octave << " degree: " << degree << endl;
				
		noteEvent->pitch.push_back(GetMidiPitch(scale, octave, degree));
		noteEvent->pitchWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
	}

	arg = args[1];
	if (arg->IsArray()) {
		Array* arr = Array::Cast(*arg);
		for (int i=0; i<arr->Length(); i++) {
			Local<Value> elem = arr->Get(i);
			if (elem->IsArray()) {
				// weighted length
				Array* pair = Array::Cast(*elem);
				short value = pair->Get(0)->Uint32Value();
				double weight = pair->Get(1)->NumberValue();
				noteEvent->velocity.push_back(value);
				noteEvent->velocityWeight.push_back(static_cast<unsigned long>(weight * WEIGHT_SCALE));
			}
			else if (elem->IsNumber()) {
				// just length
				short value = elem->Uint32Value();
				noteEvent->velocity.push_back(value);
				noteEvent->velocityWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
			}
			else {
				// error
			}
		}
	}
	else {
		noteEvent->velocity.push_back(arg->Uint32Value());
		noteEvent->velocityWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
	}

	arg = args[2];
	if (arg->IsArray()) {
		Array* arr = Array::Cast(*arg);
		for (int i=0; i<arr->Length(); i++) {
			Local<Value> elem = arr->Get(i);
			if (elem->IsArray()) {
				// weighted length
				Array* pair = Array::Cast(*elem);
				double value = pair->Get(0)->NumberValue();
				double weight = pair->Get(1)->NumberValue();
				noteEvent->length.push_back(value);
				noteEvent->lengthWeight.push_back(static_cast<unsigned long>(weight * WEIGHT_SCALE));
			}
			else if (elem->IsNumber()) {
				// just length
				double value = elem->NumberValue();
				noteEvent->length.push_back(value);
				noteEvent->lengthWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
			}
			else {
				// error
			}
		}
	}
	else {
		noteEvent->length.push_back(arg->NumberValue());
		noteEvent->lengthWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
	}

	// Fetch the template for creating JavaScript http request wrappers.
	// It only has to be created once, which we do on demand.
	if (gNoteTemplate.IsEmpty()) {
		Handle<ObjectTemplate> raw_template = MakeNoteTemplate();
		gNoteTemplate = Persistent<ObjectTemplate>::New(raw_template);
	}
	Handle<ObjectTemplate> templ = gNoteTemplate;

	// Create an empty object wrapper.
	Handle<Object> result = templ->NewInstance();

	// Wrap the raw C++ pointer in an External so it can be referenced
	// from within JavaScript.
	JSObjectHolder* eventHolder = new JSObjectHolder(noteEvent);
	Handle<External> notePtr = External::New(eventHolder);

	// Store the request pointer in the JavaScript wrapper.
	result->SetInternalField(0, notePtr);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return handle_scope.Close(result);
}

Handle<ObjectTemplate> MakeNoteTemplate() {
	HandleScope handle_scope;

	Handle<ObjectTemplate> result = ObjectTemplate::New();
	result->SetInternalFieldCount(1);

	// Add accessors for each of the fields of the request.
	result->SetAccessor(String::NewSymbol("pitch"), GetPitch);

	// Again, return the result through the current handle scope.
	return handle_scope.Close(result);
}

Handle<Value> GetPitch(Local<String> name, const AccessorInfo& info) {
	// Extract the C++ object from the JavaScript wrapper.
	JSObjectHolder* event = ExtractObjectFromJSWrapper<JSObjectHolder>(info.Holder());
	WeightedEvent* noteEvent = *boost::get<WeightedEvent*>(event);

	// Fetch the path.
	short pitch = noteEvent->pitch.at(0);

	// Wrap the result in a JavaScript string and return it.
	return Integer::New(pitch);
}

Handle<ObjectTemplate> MakeRestTemplate() {
	HandleScope handle_scope;

	Handle<ObjectTemplate> result = ObjectTemplate::New();
	result->SetInternalFieldCount(1);

	// Add accessors for each of the fields of the request.

	// Again, return the result through the current handle scope.
	return handle_scope.Close(result);
}

Handle<Value> MakeRest(const Arguments& args)
{
	HandleScope handle_scope;

	WeightedEvent* restEvent = new WeightedEvent;
	restEvent->type = REST;

	Local<Value> arg = args[0];
	if (arg->IsArray()) {
		Array* arr = Array::Cast(*arg);
		for (int i=0; i<arr->Length(); i++) {
			Local<Value> elem = arr->Get(i);
			if (elem->IsArray()) {
				// weighted length
				Array* pair = Array::Cast(*elem);
				double value = pair->Get(0)->NumberValue();
				double weight = pair->Get(1)->NumberValue();
				restEvent->length.push_back(value);
				restEvent->lengthWeight.push_back(static_cast<unsigned long>(weight * WEIGHT_SCALE));
			}
			else if (elem->IsNumber()) {
				// just length
				double value = elem->NumberValue();
				restEvent->length.push_back(value);
				restEvent->lengthWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
			}
			else {
				// error
			}
		}
	}
	else {
		restEvent->length.push_back(arg->NumberValue());
		restEvent->lengthWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
	}

	// Fetch the template for creating JavaScript http request wrappers.
	// It only has to be created once, which we do on demand.
	if (gRestTemplate.IsEmpty()) {
		Handle<ObjectTemplate> raw_template = MakeRestTemplate();
		gRestTemplate = Persistent<ObjectTemplate>::New(raw_template);
	}
	Handle<ObjectTemplate> templ = gRestTemplate;

	// Create an empty object wrapper.
	Handle<Object> result = templ->NewInstance();

	// Wrap the raw C++ pointer in an External so it can be referenced
	// from within JavaScript.
	JSObjectHolder* eventHolder = new JSObjectHolder(restEvent);
	Handle<External> ptr = External::New(eventHolder);

	// Store the request pointer in the JavaScript wrapper.
	result->SetInternalField(0, ptr);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return handle_scope.Close(result); 
}

static Handle<Value> Play(const Arguments& args) {
	HandleScope scope;
	Local<Value> arg = args[0];
	JSObjectHolder* holder = ExtractObjectFromJSWrapper<JSObjectHolder>(arg->ToObject());
	Pattern** pattern = boost::get<Pattern*>(holder);
	


	return v8::Undefined();
}

Handle<ObjectTemplate> MakePatternTemplate() {
	HandleScope handle_scope;

	Handle<ObjectTemplate> result = ObjectTemplate::New();
	result->SetInternalFieldCount(1);

	// Add accessors for each of the fields of the request.
	//result->SetAccessor(String::NewSymbol("pitch"), GetPitch);

	// Again, return the result through the current handle scope.
	return handle_scope.Close(result);
}

Handle<Value> MakePattern(const Arguments& args) {
	HandleScope handle_scope;

	Pattern* newPattern = new Pattern;

	for (int i=0; i<args.Length(); i++)
	{
		Local<Value> arg = args[i];

		if (arg->IsObject())
		{
			JSObjectHolder* holder = ExtractObjectFromJSWrapper<JSObjectHolder>(arg->ToObject());
			WeightedEvent** note = boost::get<WeightedEvent*>(holder);
			Pattern** pattern = boost::get<Pattern*>(holder);

			if (note) {
				cout << "note!" << endl;
				newPattern->Add(**note);
			}
			else if (pattern) {
				cout << "pattern!" << endl;
				newPattern->Add(**pattern);
			}
		}
		else if (arg->IsNumber())
		{
			unsigned long repeatCount = arg->Uint32Value();
			newPattern->SetRepeatCount(repeatCount);
			cout << "Set repeat count: " << repeatCount << endl;
		}
	}

	// Fetch the template for creating JavaScript http request wrappers.
	// It only has to be created once, which we do on demand.
	if (gPatternTemplate.IsEmpty()) {
		Handle<ObjectTemplate> raw_template = MakePatternTemplate();
		gPatternTemplate = Persistent<ObjectTemplate>::New(raw_template);
	}
	Handle<ObjectTemplate> templ = gPatternTemplate;

	// Create an empty object wrapper.
	Handle<Object> result = gPatternTemplate->NewInstance();

	// Wrap the raw C++ pointer in an External so it can be referenced
	// from within JavaScript.
	JSObjectHolder* eventHolder = new JSObjectHolder(newPattern);
	Handle<External> ptr = External::New(eventHolder);

	// Store the request pointer in the JavaScript wrapper.
	result->SetInternalField(0, ptr);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return handle_scope.Close(result);
}
