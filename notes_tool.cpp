
// grindtrick import googletest
// grindtrick import pstream
// grindtrick import boost_filesystem
// grindtrick import boost_system

#include <algorithm>
#include <iostream>
#include <iomanip>
#include <functional>
#include <vector>
#include <set>
#include <sstream>
#include <regex>

#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/case_conv.hpp>
#include <boost/algorithm/string/replace.hpp>
#include <boost/filesystem.hpp>
#include <boost/optional.hpp>
#include <boost/optional/optional_io.hpp>
#include <boost/range/algorithm/count.hpp>
#include <boost/range/algorithm/count_if.hpp>

using namespace std;
using namespace std::placeholders;
using namespace boost::filesystem;
using namespace boost::algorithm;
using namespace boost::range;
using boost::optional;


wstring const SUBJECT_FIELD_NAME(L"Sujet");
wstring const TAG_FIELD_NAME(L"\u00C9tiquettes");

/////////////////////////////////////////////////////////////////////////////

vector<wstring> ignores;

void load_ignore()
{
	ignores.push_back(L"\\.notesignore");

	std::wifstream fs(".notesignore");
	wstring line;
	while( getline(fs, line) )
	{
		ignores.push_back(line);
	}
}

bool is_ignored(wstring const & fn)
{
	for(auto i: ignores)
	{
		wregex re(i);
		wsmatch mr;
		
		if( regex_match(fn, mr, re) ) 
			return true;
	}
	return false;
}

/////////////////////////////////////////////////////////////////////////////

bool is_input(wstring const & t, wstring const & test_for)
{
	wstring lo(t);
	to_lower(lo);
	return lo == test_for || lo == wstring(1, test_for.front());
}

bool is_yes(wstring const & t)
{
	return is_input(t, L"yes");
}

bool is_all(wstring const & t)
{
	return is_input(t, L"all");
}

bool is_file_all_repairs(wstring const & t)
{
	return is_input(t, L"file");
}

bool is_file_skip(wstring const & t)
{
	return is_input(t, L"skip");
}

bool is_quit(wstring const & t)
{
	return is_input(t, L"quit");
}

/////////////////////////////////////////////////////////////////////////////

class File
{
public:
	File() {}
	File(path const & f, path const & a = path())
		: filename(f), annex(a) {}

	path filename;
	path annex;
};


/////////////////////////////////////////////////////////////////////////////

class Name
{
public:
	optional<wstring> sphere;
	optional<wstring> project;
	optional<wstring> subject;
};


/////////////////////////////////////////////////////////////////////////////

bool parse_filename(File const & file, Name & name_out)
{
	name_out = Name();

	wregex re(L"(\\S+) (\\S+) ([\\S ]+)");
	wsmatch mr;
	
	wstring stem = file.filename.stem().wstring();

	if( !regex_match(stem, mr, re) ) 
		return false;

	name_out.sphere  = wstring(L"#") + mr.str(1);
	name_out.project = wstring(L"#") + mr.str(2);

	wstring subject = mr.str(3);

	if( isspace(subject.front()) || isspace(subject.back()) )
		return false;

	name_out.subject = subject;

	return true;
}


/////////////////////////////////////////////////////////////////////////////

bool is_tag(wstring const & t)
{
	if( t.size() <= 1 ) return false;
	if( t[0] != L'#' ) return false;
	if( count_if( t, iswspace ) ) return false;
	return count( t.begin() + 1, t.end(), L'#' ) == 0;
}

bool parse_tags(wstring const & tags_string, set<wstring> & tags_out)
{
	tags_out.clear();
	set<wstring> t;

	wistringstream is(tags_string);

	wstring tag;
	while( is >> tag )
	{
		if( !is_tag(tag) ) return false;
		t.insert(tag);
	}

	tags_out = t;
	return true;
}

wstring print_tags(set<wstring> const & tags)
{
	wstring r;

	for(auto t: tags)
	{
		r += t + L' ';
	}

	if( !r.empty() ) 
	{
		r.erase(r.end()-1);
	}

	return r;
}


/////////////////////////////////////////////////////////////////////////////

bool parse_header_field(wstring const & line, wstring & name, wstring & body)
{
	wregex re(L"(\\S+):(.*)\n?");
	wsmatch mr;
	
	if( !regex_match(line, mr, re) ) 
		return false;

	name = mr.str(1);
	trim(name);

	body = mr.str(2);
	trim(body);

	return true;
}


/////////////////////////////////////////////////////////////////////////////

class Note
{
public:

	Note() {}

	explicit Note(File fl)
	{
		file = fl;

		parse_filename(file, name);
		load_text();
		parse_text(text);
		parse_tags();
	}

	File file;
	Name name;

	wstring text;

	map<wstring, wstring> header;
	wstring body;

	set<wstring> tags;

	void write();

	void parse_text(wstring const & text)
	{
		header.clear();
		body.clear();

		wstringstream is(text);
		wstring line;

		while( getline(is, line) )
		{
			wstring field_name;
			wstring field_body;
			if( parse_header_field(line, field_name, field_body) )
			{
				header[field_name] = field_body;
			}
			else
			{
				break;
			}
		}

		if( header.empty() )
		{
			// No header at all.  Body must include all the lines.
			body += line + L"\n";
		}
		else
		{
			// There is an header.  Do not put the (normally present)
			// empty line in the body.
			wstring tmp(line);
			trim(tmp);

			if( !tmp.empty() )
				body += line + L"\n";
		}
		
		while( getline(is, line) )
			body += line + L"\n";
	}

private:
	void load_text()
	{
		std::wifstream fs(file.filename.string());
		wstringstream strstream;
		strstream << fs.rdbuf();
		text = strstream.str();
	}

	void parse_tags()
	{
		auto end = header.end();

		if( header.find(TAG_FIELD_NAME) != end )
		{
			wstring body = header[TAG_FIELD_NAME];
			::parse_tags( body, tags );
		}
	}
};

void Note::write()
{
	std::wofstream fs(file.filename.string());

	for(auto field: header)
	{
		fs << field.first << ": " << field.second << '\n';
	}

	if( !header.empty() ) fs << '\n';

	fs << body;
}


///////////////////////////////////////////////////////////////////////

wostream & operator <<(wostream & os, Note const & n)
{
	os << "Note(" << n.file.filename << ")";
	return os;
}


///////////////////////////////////////////////////////////////////////


class DirectoryVisitor
{
public:
	virtual ~DirectoryVisitor() {}
	virtual bool directory(path p) = 0;
	virtual bool file(File const &) = 0;
};


bool visit(path dir, DirectoryVisitor & visitor)
{
	vector<path> dirs;
	vector<path> filepaths;
	for(auto x : directory_iterator(dir))
	{
		path p = x.path();
		auto fn = p.filename();

		if( is_ignored(fn.wstring()) ) continue;

		if( is_directory   (x) )      dirs.push_back(x);
		if( is_regular_file(x) ) filepaths.push_back(x);
	}


	vector<File> files;

	for(auto filepath: filepaths)
	{
		File file(filepath);

		// TODO: check for "annexes" extension
		auto predicate = [fn = file.filename](auto dir)
			{ return dir.stem() == fn.stem(); };

		auto it = find_if(dirs.begin(), dirs.end(), predicate);

		if( it != dirs.end() )
		{
			file.annex = *it;
			dirs.erase(it);
		}

		files.push_back(file);
	}

	for(auto dir: dirs) 
	{
		if( !visitor.directory(dir) ) return false;
	}

	for(auto file: files) 
	{
		if( !visitor.file(file) ) return false;
	}

	return true;
}


///////////////////////////////////////////////////////////////////////

class BaseCheck
{
public:
	explicit BaseCheck(Note const & note) : note_(note) {}

	operator bool () const { return msg_.empty(); }

	wstring message() const { return msg_; }

protected:
	Note note_;
	wstring msg_;
};


class HasSubjectFieldCheck : public BaseCheck
{
public:
	explicit HasSubjectFieldCheck(Note const & note) : BaseCheck(note) 
	{
		auto end = note_.header.end();
		auto subject_field = note_.header.find(SUBJECT_FIELD_NAME);

		if( subject_field == end )
		{
			wostringstream os;
			os << L"missing \"" << SUBJECT_FIELD_NAME << "\" header";
			msg_ = os.str();
		}
		else
		{
			subject_ = (*subject_field).second;
		}
	}

	wstring subject() const { return subject_; }

protected:
	wstring subject_;
};


class MatchingSubjectsCheck : public BaseCheck
{
public:

	explicit MatchingSubjectsCheck(Note const & note) : 
		BaseCheck(note)
	{
		HasSubjectFieldCheck has_subject(note);

		auto name_subject = note.name.subject;

		if( name_subject ) subject_ = *name_subject;

		if( has_subject && name_subject )
		{
			if( *name_subject != has_subject.subject() )
			{
				msg_ = L"subject mismatch";
			}
		}
	}

	wstring subject() const { return subject_; }

protected:
	wstring subject_;
};


class NonEmptyAnnexCheck : public BaseCheck
{
public:
	explicit NonEmptyAnnexCheck(Note const & note) : BaseCheck(note)
	{
		auto annex = note_.file.annex;
		if( !annex.empty() )
		{
			if( boost::filesystem::is_empty(annex) )
			{
				wstringstream ss;
				ss << L"annex is empty: " << annex.wstring();
				msg_ = ss.str();
			}
		}
	}
};

class ExtensionCheck : public BaseCheck
{
public:
	explicit ExtensionCheck(Note const & note) : BaseCheck(note)
	{
		if( note_.file.filename.extension() != ".md" )
		{
			msg_ = L"wrong extension";
		}
	}
};

class FilenameCheck : public BaseCheck
{
public:
	explicit FilenameCheck(Note const & note) : BaseCheck(note)
	{
		Name nm;
		if( ! parse_filename(note_.file, nm) )
		{
			msg_ = L"filename format";
		}
	}
};

class HasTagsFieldCheck : public BaseCheck
{
public:
	explicit HasTagsFieldCheck(Note const & note) : BaseCheck(note)
	{
		auto end = note_.header.end();

		if( note_.header.find(TAG_FIELD_NAME) == end )
		{
			wostringstream os;
			os << L"missing \"" << TAG_FIELD_NAME << "\" header";
			msg_ = os.str();
		}
	}
};

class EolCheck : public BaseCheck
{
public:
	explicit EolCheck(Note const & note) : BaseCheck(note)
	{
 		if( count(note_.text, L'\r') != 0 )
		{
			msg_ = L"CR detected";
		}
	}
};

class BaseFilenameTagCheck : public BaseCheck
{
public:
	BaseFilenameTagCheck(Note const & note,
		optional<wstring> const & fn_tag, wstring const & tag_desc) 
		: BaseCheck(note)
	{
		if( fn_tag )
		{
			if( count(note_.tags, *fn_tag) == 0 )
			{
				msg_ = tag_desc + L" from filename not found in tags";
			}
		}
	}
};

class SphereFilenameTagCheck : public BaseFilenameTagCheck
{
public:
	SphereFilenameTagCheck(Note const & note) :
		BaseFilenameTagCheck(note, note.name.sphere, L"sphere of life")
	{}
};

class ProjectFilenameTagCheck : public BaseFilenameTagCheck
{
public:
	ProjectFilenameTagCheck(Note const & note) :
		BaseFilenameTagCheck(note, note.name.project, L"project")
	{}
};

///////////////////////////////////////////////////////////////////////

class BaseHealer
{
public:

	explicit BaseHealer(Note & note) : note_(note) {}

protected:
	Note & note_;
};

class SubjectFieldHealer : public BaseHealer
{
public:

	explicit SubjectFieldHealer(Note & note)
		: BaseHealer(note), matching_(note), has_(note) {}

	operator bool () const
	{
		if( !has_ ) return false;
		if( !matching_ ) return false;
		return true;
	}

	wstring message() const
	{
		if( !has_ ) return has_.message();
		return matching_.message();
	}

	void heal()
	{
		note_.header[SUBJECT_FIELD_NAME] = matching_.subject();
		note_.write();
	}

protected:
	MatchingSubjectsCheck matching_;
	HasSubjectFieldCheck has_;
};

class TagsFieldHealer : public BaseHealer
{
public:
	explicit TagsFieldHealer(Note & note) : BaseHealer(note),
		has_(note), filename_(note), sphere_(note), project_(note)
	{}

	operator bool() const
	{
		if( filename_ )
		{
			if( !has_ || !sphere_ || !project_ )
			{
				return false;
			}
		}
		// else, cannot heal.
		return true;
	}

	wstring message() const
	{
		if( filename_ )
		{
			if( !has_ )
			{
				return has_.message();
			}
			else if( !sphere_ || !project_ )
			{
				return L"tag(s) from filename are missing";
			}
		}
		// else, cannot heal.
		return wstring();
	}

	void heal()
	{
		note_.tags.insert(*note_.name.sphere);
		note_.tags.insert(*note_.name.project);
		note_.header[TAG_FIELD_NAME] = print_tags(note_.tags);
		note_.write();
	}

private:
	HasTagsFieldCheck has_;
	FilenameCheck filename_;
	SphereFilenameTagCheck sphere_;
	ProjectFilenameTagCheck project_;
};

class EolHealer : public BaseHealer
{
public:
	explicit EolHealer(Note & note) : BaseHealer(note),
		eol_(note)
	{}

	operator bool () const
	{
		return eol_;
	}

	wstring message() const
	{
		return eol_.message();
	}

	void heal()
	{
		erase_all(note_.text, L"\r");
		erase_all(note_.body, L"\r");
		note_.write();
	}

private:
	EolCheck eol_;
};


///////////////////////////////////////////////////////////////////////

class BaseDirectoryVisitor : public DirectoryVisitor
{
public:
	void print_tags()
	{
		print_tags(L"Sphere of life", sphere_tags);
		wcout << '\n';
		print_tags(L"Project", project_tags);
		wcout << '\n';
		print_tags(L"Tags", tags);
	}

protected:
	Note load_note(File const & file)
	{
		Note note(file);

		accumulate_tags(note);

		return note;
	}

	template <typename CheckType>
	void check(Note const & note)
	{
		CheckType check(note);
		if( !check )
		{
			print_warning(check.message(), note);
		}
	}

	void print_warning(wstring msg)
	{
		wcout << "warning: " << msg << '\n';
	}

	void print_warning(wstring msg, Note note)
	{
		wcout << "warning(" << note.file.filename.wstring() << "): ";
		wcout << msg << '\n';
	}

	map< wstring, int > sphere_tags;
	map< wstring, int > project_tags;
	map< wstring, int > tags;

private:
	void print_tags(wstring const & name, map<wstring, int> tags)
	{
		wcout << name << ":\n";
		if( tags.empty() )
		{
			wcout << L"  <no tags>\n";
		}
		else
		{
			for(auto const & t: tags) 
			{
				wcout << "  ";
				wcout << setw(20) << left << t.first; 
				wcout << setw(4) << right << t.second;
				wcout << '\n';
			}
		}
	}

	void accumulate_tags(Note const & note)
	{
		if( note.name.sphere  ) ++  sphere_tags[*note.name. sphere] ;
		if( note.name.project ) ++ project_tags[*note.name.project] ;

		for(wstring tag: note.tags)
		{
			bool is_sphere  =  sphere_tags.count(tag) != 0;
			bool is_project = project_tags.count(tag) != 0;

			if( !is_sphere && !is_project ) ++ tags[tag] ;
		}
	}
};



class WarningVisitor : public BaseDirectoryVisitor
{
public:
	virtual bool directory(path dir)
	{
		wstringstream ss;
		ss << L"orphan directory found: " << dir.wstring();
		print_warning(ss.str());
		return true;
	}

	virtual bool file(File const & file)
	{
		Note note = load_note(file);

		check<NonEmptyAnnexCheck>(note);

		check<ExtensionCheck>(note);

		check<FilenameCheck>(note);

		check<HasSubjectFieldCheck>(note);

		check<HasTagsFieldCheck>(note);

		check<MatchingSubjectsCheck>(note);

		check<EolCheck>(note);

		check<SphereFilenameTagCheck>(note);
		check<ProjectFilenameTagCheck>(note);

		return true;
	}
};

class PrintTagsVisitor : public BaseDirectoryVisitor
{
public:
	virtual bool directory(path)
	{
		return true;
	}

	virtual bool file(File const & file)
	{
		Note note = load_note(file);
		return true;
	}
};

class HealerVisitor : public BaseDirectoryVisitor
{
	struct quit_signal {};

public:
	virtual bool directory(path)
	{
		return true;
	}

	virtual bool file(File const & file)
	{
		try
		{
			Note note = load_note(file);

			if( heal<EolHealer>(note) )
			{
				note = load_note(file);
			}

			heal<SubjectFieldHealer>(note);

			heal<TagsFieldHealer>(note);

			if( !is_all(input_) )
			{
				input_.clear();
			}
		}
		catch(quit_signal const &)
		{
			return false;
		}

		return true;
	}

private:
	template <typename Healer>
	bool heal(Note & note)
	{
		if( is_file_skip(input_) )
		{
			return true;
		}

		Healer h(note);
		if( !h )
		{
			wcout << note.file.filename.wstring() << ": " << h.message() << '\n';

			/*
			yes - this file, this repair only
			no - skip this file, this repair only
			file - all repairs for this file only
			skip - skip all repairs for this file only
			all - all files, all repairs
			quit
			*/

			if( !is_all(input_) && !is_file_all_repairs(input_) )
			{
				wcout << "Repair? (y|[n]|file|skip|all|quit) " << flush;
				getline(wcin, input_);
			}

			if( is_yes(input_) || is_file_all_repairs(input_) || is_all(input_) )
			{
				h.heal();
				wcout << "REPAIRED!\n";
				return true;
			}
			else if( is_quit(input_) )
			{
				throw quit_signal();
			}
		}
		return false;
	}

	wstring input_;
};


///////////////////////////////////////////////////////////////////////

int normal_main(int, char **)
{
	WarningVisitor visitor;
	visit(".", visitor);

	return 0;
}

int print_tags_main(int,	 char **)
{
	PrintTagsVisitor visitor;
	visit(".", visitor);

	visitor.print_tags();

	return 0;
}

int heal_main(int, char **)
{
	HealerVisitor visitor;
	visit(".", visitor);
	return 0;
}

int tests(int argc, char ** argv);

int user_main(int argc, char ** argv)
{
	load_ignore();

	bool is_tests = argc == 2 && string(argv[1]) == "tests";
	bool is_tags  = argc == 2 && string(argv[1]) == "tags";
	bool is_heal  = argc == 2 && string(argv[1]) == "repair";

	if( is_tests )
	{
		return tests(argc, argv);
	}
	else if( is_heal )
	{
		return heal_main(argc, argv);
	}
	else if( is_tags )
	{
		return print_tags_main(argc, argv);
	}
	else
	{
		return normal_main(argc, argv);
	}
}

int main(int argc, char ** argv)
{
	// Load the system locale.
	setlocale(LC_ALL, "");

	try
	{
		return user_main(argc, argv);
	}	
	catch(exception const & ex)
	{
		cerr << "exception: " << ex.what() << '\n';
		return 1;
	}
}

#include "notes_tool_tests.cpp"
