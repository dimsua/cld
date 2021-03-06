
#include <string>
#include <v8.h>
#include <node.h>
#include <node_version.h>

// Include the CLD headers
#include "cld/compact_lang_det.h"
#include "cld/encodings/compact_lang_det/ext_lang_enc.h"
#include "cld/encodings/compact_lang_det/unittest_data.h"
#include "cld/encodings/proto/encodings.pb.h"

using namespace node;
using namespace v8;

#define NUM_POSSIBLE_LANGUAGES 3

class LanguageDetector: ObjectWrap
{
private:
	// Convert a V8 string to a UTF8 string.
	static std::string GetString(v8::Handle<v8::String> str)
	{
	  // Allocate enough space for a worst-case conversion.
	  int len = str->Utf8Length();
	  char* buf = new char[len+1];
	  str->WriteUtf8(buf, len+1);
	  std::string ret(buf, len);
	  delete [] buf;
	  return ret;
	}

	static bool optionalBooleanArg(const Arguments& args, int position, const char* key, bool default_value)
	{
		bool result = default_value;
		if (position > 0 && args.Length() > position)
		{
			Local<Object> hash = args[position]->ToObject();
			Handle<String> v8Key = String::New(key);
			if ( hash->Has(v8Key) )
			{
				result = hash->Get(v8Key)->ToBoolean()->Value();
		    }
		}
		return result;
	}

	static const char* optionalStringArg(const Arguments& args, int position, const char* key, const char* default_value)
	{
		const char* result = default_value;
		if (position > 0 && args.Length() > position)
		{
			Local<Object> hash = args[position]->ToObject();
			Handle<String> v8Key = String::New(key);
			if ( hash->Has(v8Key) )
			{
				v8::String::Utf8Value str(hash->Get(v8Key)->ToString());
				result = *str;
		    }
		}
		return result;
	}

public:

  static Persistent<FunctionTemplate> s_ct;
  static void Init(Handle<Object> target)
  {
    HandleScope scope;

    Local<FunctionTemplate> t = FunctionTemplate::New(New);

    s_ct = Persistent<FunctionTemplate>::New(t);
    s_ct->InstanceTemplate()->SetInternalFieldCount(1);
    s_ct->SetClassName(String::NewSymbol("LanguageDetector"));

    NODE_SET_PROTOTYPE_METHOD(s_ct, "detectSync", DetectSync);
    NODE_SET_PROTOTYPE_METHOD(s_ct, "detect", DetectAsync);

    target->Set(String::NewSymbol("LanguageDetector"),
                s_ct->GetFunction());
  }

  LanguageDetector()
  {
  }

  ~LanguageDetector()
  {
  }

  static Handle<Value> New(const Arguments& args)
  {
    HandleScope scope;
    LanguageDetector* hw = new LanguageDetector();
    hw->Wrap(args.This());
    return args.This();
  }

  static Handle<Value> DetectSync(const Arguments& args)
  {
    HandleScope scope;
  	Handle<String> v8text = args[0]->ToString();
  	const char *src = GetString(v8text).c_str();

  	// Parse options.
  	bool is_plain_text = !optionalBooleanArg(args, 1, "html", false);
    bool do_allow_extended_languages = optionalBooleanArg(args, 1, "allowExtendedLanguages", true);
    bool do_pick_summary_language = optionalBooleanArg(args, 1, "pickSummaryLanguage", false);
    bool do_remove_weak_matches = optionalBooleanArg(args, 1, "skipWeakMatches", false);
    const char* tld_hint = optionalStringArg(args, 1, "tld", NULL);

    bool is_reliable;
    int encoding_hint = UNKNOWN_ENCODING;
    Language language_hint = UNKNOWN_LANGUAGE;

    double normalized_score3[NUM_POSSIBLE_LANGUAGES];
    Language language3[NUM_POSSIBLE_LANGUAGES];
    int percent3[NUM_POSSIBLE_LANGUAGES];
    int text_bytes;

    Language lang;
    lang = CompactLangDet::DetectLanguage(0,
                                          src, strlen(src),
                                          is_plain_text,
                                          do_allow_extended_languages,
                                          do_pick_summary_language,
                                          do_remove_weak_matches,
                                          tld_hint,
                                          encoding_hint,
                                          language_hint,
                                          language3,
                                          percent3,
                                          normalized_score3,
                                          &text_bytes,
                                          &is_reliable);

    // Dummy result.
    Local<String> result = String::New(LanguageCode(lang));
    return scope.Close(result);
  }

  struct language_detect_result_t {
	  std::string language_code;
	  bool reliable;
	  double normalized_scores[NUM_POSSIBLE_LANGUAGES];
    Language languages[NUM_POSSIBLE_LANGUAGES];
    int percents[NUM_POSSIBLE_LANGUAGES];
  };

  struct language_detect_baton_t {
    LanguageDetector *ld;

	  std::string source_text;
	  std::string tld_hint;
	  bool is_plain_text;
    bool do_allow_extended_languages;
    bool do_pick_summary_language;
    bool do_remove_weak_matches;

    language_detect_result_t *result;
    Persistent<Function> cb;
  };

  static Handle<Value> DetectAsync(const Arguments& args)
  {
		HandleScope scope;
		int options_arg_idx = -1;
		Local<Function> cb;

		if (args.Length() == 3 && args[2]->IsFunction())
		{
			options_arg_idx = 1;
			cb = Local<Function>::Cast(args[2]);
		}
		else if (args.Length() == 2 && args[1]->IsFunction())
		{
			cb = Local<Function>::Cast(args[1]);
		}
		else
		{
			return ThrowException(Exception::TypeError(String::New("Argument 2 or 3 must be a function")));
		}

	  LanguageDetector* ld = ObjectWrap::Unwrap<LanguageDetector>(args.This());
		language_detect_baton_t *baton = new language_detect_baton_t();
	  baton->ld = ld;

		baton->source_text = GetString(args[0]->ToString());
		baton->tld_hint =  optionalStringArg(args, options_arg_idx, "tld", "");
		baton->is_plain_text = !optionalBooleanArg(args, options_arg_idx, "html", false);
    baton->do_allow_extended_languages = optionalBooleanArg(args, options_arg_idx, "allowExtendedLanguages", true);
    baton->do_pick_summary_language = optionalBooleanArg(args, options_arg_idx, "pickSummaryLanguage", false);
    baton->do_remove_weak_matches = optionalBooleanArg(args, options_arg_idx, "skipWeakMatches", false);
    // baton->tld_hint =  Persistent<String>::New(optionalStringArg(args, options_arg_idx, "tld", NULL));
    baton->cb = Persistent<Function>::New(cb);

	  ld->Ref();

		// eio_custom(EIO_Detect, EIO_PRI_DEFAULT, EIO_AfterDetect, baton);
		// ev_ref(EV_DEFAULT_UC);

    uv_work_t *req = new uv_work_t;
    req->data = baton;

    uv_queue_work(uv_default_loop(), req, uv_detect, uv_after_detect);

		return Undefined();
  }

  static void uv_detect(uv_work_t *request)
  {
    language_detect_baton_t *baton = static_cast<language_detect_baton_t *>(request->data);
    baton->result = new language_detect_result_t();

    const char *src = baton->source_text.c_str();
    const char *tld_hint = NULL;
    if (!baton->tld_hint.empty())
    {
      tld_hint = baton->tld_hint.c_str();
    }

    int encoding_hint = UNKNOWN_ENCODING;
    Language language_hint = UNKNOWN_LANGUAGE;
    int text_bytes;

    Language lang;
    lang = CompactLangDet::DetectLanguage(0,
                                          src, strlen(src),
                                          baton->is_plain_text,
                                          baton->do_allow_extended_languages,
                                          baton->do_pick_summary_language,
                                          baton->do_remove_weak_matches,
                                          tld_hint,
                                          encoding_hint,
                                          language_hint,
                                          baton->result->languages,
                                          baton->result->percents,
                                          baton->result->normalized_scores,
                                          &text_bytes,
                                          &baton->result->reliable);

    std::string lang_str(LanguageCode(lang));
    baton->result->language_code = lang_str;
  }

  static void uv_after_detect(uv_work_t *request, int status)
  {
    HandleScope scope;
    language_detect_baton_t *baton = static_cast<language_detect_baton_t *>(request->data);
    baton->ld->Unref();

    // Get the result information from the baton
    Local<Value> argv[1];
    Local<Object> resultObject = Object::New();
    resultObject->Set(String::New("languageCode"), String::New(baton->result->language_code.c_str()));
    resultObject->Set(String::New("reliable"), Boolean::New(baton->result->reliable));

      Handle<Array> detailArray = Array::New(NUM_POSSIBLE_LANGUAGES);
    for (int i=0; i < NUM_POSSIBLE_LANGUAGES; i++)
    {
      // Build a detail object
      Local<Object> detailObject = Object::New();
      std::string lang_str(LanguageCode(baton->result->languages[i]));
      detailObject->Set(String::New("languageCode"), String::New(lang_str.c_str()));
      detailObject->Set(String::New("normalizedScore"), Number::New(baton->result->normalized_scores[i]));
      detailObject->Set(String::New("percentScore"), Number::New(baton->result->percents[i]));
      // Put the result on the details array
      detailArray->Set(i, detailObject);
    }
    resultObject->Set(String::New("details"), detailArray);
    argv[0] = resultObject;


    TryCatch try_catch;
    // Call the callback function.
    baton->cb->Call(Context::GetCurrent()->Global(), 1, argv);

    if (try_catch.HasCaught()) {
      FatalException(try_catch);
    }

    baton->cb.Dispose();

    delete baton;
  }

};

Persistent<FunctionTemplate> LanguageDetector::s_ct;

extern "C" {
  static void init (Handle<Object> target)
  {
    LanguageDetector::Init(target);
  }

  NODE_MODULE(cld, init);
}
