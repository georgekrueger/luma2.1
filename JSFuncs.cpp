
#include "JSFuncs.h"
#include "music.h"
#include <assert.h>
#include <iostream>
#include <sstream>

using namespace v8;

// Note
static Persistent<ObjectTemplate> gNoteTemplate;
v8::Handle<v8::Value> MakeNote(const v8::Arguments& args);
Handle<ObjectTemplate> MakeNoteTemplate();
WeightedEvent* UnwrapNote(Handle<Object> obj);
Handle<Object> WrapNote(WeightedEvent& event);
Handle<Value> GetPitch(Local<String> name, const AccessorInfo& info);

Persistent<Context> CreateV8Context()
{
	v8::HandleScope handle_scope;

	v8::Handle<v8::ObjectTemplate> global = v8::ObjectTemplate::New();

	// Bind the global 'print' function to the C++ Print callback.
	global->Set(v8::String::New("print"), v8::FunctionTemplate::New(Print));
	global->Set(v8::String::New("note"), v8::FunctionTemplate::New(MakeNote));

	v8::Persistent<v8::Context> context = v8::Context::New(NULL, global);

	return context;
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
	
	Local<Value> arg;

	WeightedEvent event;

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
				event.pitch.push_back(GetMidiPitch(scale, octave, degree));
				event.pitchWeight.push_back(static_cast<unsigned long>(weight * WEIGHT_SCALE));
			}
			else if (elem->IsNumber()) {
				
				Scale scale;
				short octave;
				short degree;
				ExtractPitch(elem, scale, octave, degree);
				cout << "Scale: " << scale << " octave: " << octave << " degree: " << degree << endl;
				
				event.pitch.push_back(GetMidiPitch(scale, octave, degree));
				event.pitchWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
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
				
		event.pitch.push_back(GetMidiPitch(scale, octave, degree));
		event.pitchWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
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
				event.velocity.push_back(value);
				event.velocityWeight.push_back(static_cast<unsigned long>(weight * WEIGHT_SCALE));
			}
			else if (elem->IsNumber()) {
				// just length
				short value = elem->Uint32Value();
				event.velocity.push_back(value);
				event.velocityWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
			}
			else {
				// error
			}
		}
	}
	else {
		event.velocity.push_back(arg->NumberValue());
		event.velocityWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
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
				event.length.push_back(value);
				event.lengthWeight.push_back(static_cast<unsigned long>(weight * WEIGHT_SCALE));
			}
			else if (elem->IsNumber()) {
				// just length
				double value = elem->NumberValue();
				event.length.push_back(value);
				event.lengthWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
			}
			else {
				// error
			}
		}
	}
	else {
		event.length.push_back(arg->NumberValue());
		event.lengthWeight.push_back(static_cast<unsigned long>(1 * WEIGHT_SCALE));
	}

	Handle<Object> note = WrapNote(event);
	return note;
}

Handle<ObjectTemplate> MakeNoteTemplate() {
	HandleScope handle_scope;

	Handle<ObjectTemplate> result = ObjectTemplate::New();
	result->SetInternalFieldCount(1);

	// Add accessors for each of the fields of the request.
	result->SetAccessor(String::NewSymbol("pitch"), GetPitch);
	//result->SetAccessor(String::NewSymbol("referrer"), GetReferrer);
	//result->SetAccessor(String::NewSymbol("host"), GetHost);
	//result->SetAccessor(String::NewSymbol("userAgent"), GetUserAgent);

	// Again, return the result through the current handle scope.
	return handle_scope.Close(result);
}

/**
 * Utility function that extracts the C++ http request object from a
 * wrapper object.
 */
WeightedEvent* UnwrapNote(Handle<Object> obj) {
	Handle<External> field = Handle<External>::Cast(obj->GetInternalField(0));
	void* ptr = field->Value();
	return static_cast<WeightedEvent*>(ptr);
}

Handle<Object> WrapNote(WeightedEvent& event) {
	// Handle scope for temporary handles.
	HandleScope handle_scope;

	// Fetch the template for creating JavaScript http request wrappers.
	// It only has to be created once, which we do on demand.
	if (gNoteTemplate.IsEmpty()) {
	Handle<ObjectTemplate> raw_template = MakeNoteTemplate();
	gNoteTemplate = Persistent<ObjectTemplate>::New(raw_template);
	}
	Handle<ObjectTemplate> templ = gNoteTemplate;

	// Create an empty http request wrapper.
	Handle<Object> result = templ->NewInstance();

	WeightedEvent* note = new WeightedEvent(event);
	note->type = NOTE_ON;

	// Wrap the raw C++ pointer in an External so it can be referenced
	// from within JavaScript.
	Handle<External> notePtr = External::New(note);

	// Store the request pointer in the JavaScript wrapper.
	result->SetInternalField(0, notePtr);

	// Return the result through the current handle scope.  Since each
	// of these handles will go away when the handle scope is deleted
	// we need to call Close to let one, the result, escape into the
	// outer handle scope.
	return handle_scope.Close(result);
}

Handle<Value> GetPitch(Local<String> name, const AccessorInfo& info) {
	// Extract the C++ object from the JavaScript wrapper.
	WeightedEvent* event = UnwrapNote(info.Holder());

	// Fetch the path.
	short pitch = event->pitch.at(0);

	// Wrap the result in a JavaScript string and return it.
	return Integer::New(pitch);
}
