#include <gtest/gtest.h>

TEST(parse_filename, simple)
{
	File file(L"./inro desktop The subject.md");
	Name fi;

	ASSERT_TRUE( parse_filename(file, fi) );

	ASSERT_TRUE( fi.sphere );
	EXPECT_EQ( *fi.sphere, L"#inro" );

	ASSERT_TRUE( fi.project );
	EXPECT_EQ( *fi.project, L"#desktop" );

	ASSERT_TRUE( fi.subject );
	EXPECT_EQ( *fi.subject, L"The subject" );
}

TEST(parse_filename, accented)
{
	File file(L"./inro desktop Arrêt.md");
	Name fi;

	ASSERT_TRUE( parse_filename(file, fi) );

	ASSERT_TRUE( fi.subject );
	EXPECT_EQ( *fi.subject, L"Arrêt" );
}

TEST(parse_filename, any_character)
{
	File file(L"./+=*\\ d!@#$%^&() ~`|,.<>{}[].md");
	Name fi;

	ASSERT_TRUE( parse_filename(file, fi) );

	ASSERT_TRUE( fi.sphere );
	EXPECT_EQ( *fi.sphere, L"#+=*\\" );

	ASSERT_TRUE( fi.project );
	EXPECT_EQ( *fi.project, L"#d!@#$%^&()" );

	ASSERT_TRUE( fi.subject );
	EXPECT_EQ( *fi.subject, L"~`|,.<>{}[]" );
}

TEST(parse_filename, empty_string)
{
	File file;
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_filename, start_with_space)
{
	File file(L"./ inro desktop Arrêt.md");
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_filename, end_with_space)
{
	File file(L"./inro desktop Arrêt .md");
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_filename, no_subject)
{
	File file(L"./inro desktop.md");
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_filename, no_project)
{
	File file(L"./inro.md");
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_filename, no_basename)
{
	File file(L"./.md");
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_filename, subject_starts_with_space)
{
	File file(L"./inro desktop  Two spaces.md");
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_filename, subject_is_space)
{
	File file(L"./inro desktop  .md");
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_filename, two_spaces_between_sphere_and_project)
{
	File file(L"./inro  desktop Two spaces.md");
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_filename, no_tabs)
{
	File file(L"./in\tro desktop OK.md");
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_filename, no_carriage_return)
{
	File file(L"./in\ro desktop OK.md");
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_filename, no_line_feed)
{
	File file(L"./in\nro desktop OK.md");
	Name fi;
	EXPECT_FALSE( parse_filename(file, fi) );
}

TEST(parse_header_field, empty)
{
	wstring name, body;
	EXPECT_FALSE( parse_header_field( L"", name, body ) );
	EXPECT_FALSE( parse_header_field( L"\n", name, body ) );
	EXPECT_FALSE( parse_header_field( L"   \n", name, body ) );
}

TEST(parse_header_field, not_a_field)
{
	wstring name, body;
	EXPECT_FALSE( parse_header_field( L"# Subject", name, body ) );
	EXPECT_FALSE( parse_header_field( L"# Subject\n", name, body ) );
}

TEST(parse_header_field, fields)
{
	wstring name, body;
	ASSERT_TRUE( parse_header_field( L"Sujet: le sujet", name, body ) );
	EXPECT_EQ( name, L"Sujet" );
	EXPECT_EQ( body, L"le sujet" );

	ASSERT_TRUE( parse_header_field( L"Sujet: le sujet\n", name, body ) );
	EXPECT_EQ( name, L"Sujet" );
	EXPECT_EQ( body, L"le sujet" );

	ASSERT_TRUE( parse_header_field( L"Sujet: avec deux : points\n", name, body ) );
	EXPECT_EQ( name, L"Sujet" );
	EXPECT_EQ( body, L"avec deux : points" );
}

TEST( parse_note_text, empty )
{
	Note n;

	n.parse_text( L"" );

	EXPECT_EQ( n.header.size(), 0 );
	EXPECT_EQ( n.body, L"\n" );

	n.parse_text( L"\n" );

	EXPECT_EQ( n.header.size(), 0 );
	EXPECT_EQ( n.body, L"\n" );
}

TEST( parse_note_text, with_fields )
{
	Note n;

	n.parse_text( 
		L"Sujet: le sujet\n"
		L"Etiquettes: #inro #desktop\n"
		L"\n"
		L"Le corps\n"
		L"est ici.\n");

	EXPECT_EQ( n.header.size(), 2 );

	EXPECT_EQ( n.header[L"Sujet"], L"le sujet" );
	EXPECT_EQ( n.header[L"Etiquettes"], L"#inro #desktop" );

	EXPECT_EQ( n.body, L"Le corps\nest ici.\n" );
}

TEST( parse_tags, empty )
{
	set<wstring> tags;

	ASSERT_TRUE( parse_tags(L"", tags) );
}

TEST( parse_tags, bad )
{
	set<wstring> tags;

	ASSERT_FALSE( parse_tags(L"inro", tags) );
	ASSERT_FALSE( parse_tags(L"#inro #", tags) );
	ASSERT_FALSE( parse_tags(L"#hash#hash", tags) );
}

TEST( parse_tags, valid )
{
	set<wstring> tags;

	ASSERT_TRUE( parse_tags(L"#inro #desktop", tags) );
	EXPECT_EQ( tags.size(), 2 );
	EXPECT_EQ( tags.count(L"#inro"), 1 );
	EXPECT_EQ( tags.count(L"#desktop"), 1 );

	ASSERT_TRUE( parse_tags(L"#inro   #spaces", tags) );
	EXPECT_EQ( tags.size(), 2 );
	EXPECT_EQ( tags.count(L"#inro"), 1 );
	EXPECT_EQ( tags.count(L"#spaces"), 1 );
}

TEST( is_tag, test )
{
	ASSERT_TRUE( is_tag(L"#inro") ) ;
	ASSERT_FALSE( is_tag(L"#") ) ;
	ASSERT_FALSE( is_tag(L"") ) ;
	ASSERT_FALSE( is_tag(L"#in#ro") ) ;
	ASSERT_FALSE( is_tag(L"#in ro") ) ;
}

int tests(int argc, char ** argv)
{
	testing::InitGoogleTest(&argc, argv);
  	return RUN_ALL_TESTS();
}
