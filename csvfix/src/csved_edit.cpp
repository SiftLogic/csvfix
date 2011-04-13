//---------------------------------------------------------------------------
// csved_edit.cpp
//
// Edit fields for CSVfix. This was originally going to be the primary
// user interface, but it turned out not to be such a good idea, and only
// the s (substitute) command is actually implemented.
//
// Copyright (C) 2009 Neil Butterworth
//---------------------------------------------------------------------------

#include "a_base.h"
#include "a_collect.h"
#include "a_regex.h"
#include "csved_cli.h"
#include "csved_edit.h"
#include "csved_strings.h"

using std::string;
using std::vector;

namespace CSVED {

//---------------------------------------------------------------------------
// Register find command
//---------------------------------------------------------------------------

static RegisterCommand <EditCommand> rc1_(
	CMD_EDIT,
	"edit fields"
);

//----------------------------------------------------------------------------
// Help text
//----------------------------------------------------------------------------

const char * const EDIT_HELP = {
	"performs sed-style editing on CSV data\n"
	"usage: csvfix edit [flags] [file ...]\n"
	"where flags are:\n"
	"  -f fields\tfields to apply edits to (default is al fields)\n"
	"  -e cmd\tspecify edit command - currently only s(ubstitute) implemented\n"
	"#ALL"
};

//------------------------------------------------------------------------
// Standard command ctor
//---------------------------------------------------------------------------

EditCommand ::EditCommand( const string & name,
								const string & desc )
		: Command( name, desc, EDIT_HELP ) {

	AddFlag( ALib::CommandLineFlag( FLAG_EDIT, true, 1, true ) );
	AddFlag( ALib::CommandLineFlag( FLAG_COLS, false, 1, false ) );
}

//---------------------------------------------------------------------------
// Get the subcommands and then apply them to inputs
//---------------------------------------------------------------------------

int EditCommand :: Execute( ALib::CommandLine & cmd ) {

	for ( int i = 2; i < cmd.Argc(); i++ ) {
		if ( cmd.Argv( i ) == FLAG_EDIT ) {
			AddSubCmd( cmd.Argv( i + 1 ) );
			i++;
		}
	}

	ALib::CommaList cl( cmd.GetValue( FLAG_COLS, "" ) );
	CommaListToIndex( cl, mCols );

	IOManager io( cmd );
	CSVRow row;

	while( io.ReadCSV( row ) ) {
		EditRow( row );
		io.WriteRow( row );
	}

	return 0;
}

//---------------------------------------------------------------------------
// Edit row by allplying all sub-commands to specified columns
//---------------------------------------------------------------------------

void EditCommand :: EditRow( CSVRow & row ) {
	for ( unsigned int i = 0; i < row.size(); i++ ) {
		if ( mCols.size() == 0 || ALib::Contains( mCols, i ) ) {
			EditField( row[i] );
		}
	}
}

//---------------------------------------------------------------------------
// Edit single field using suncommands. Currently only the 's' command
// is supported, which works like that in vi. e.g.
//
//		s/abc/XXX/g
//
// would change all occurrences of abc into XXX
//---------------------------------------------------------------------------

const char SUB_CMD 	= 's';		// substitute command
const char IC_OPT		= 'i';		// ignore case option
const char ALL_OPT		= 'g';		// replace all (global) option

void EditCommand :: EditField( std::string & f ) {
	for ( unsigned int i = 0; i < mSubCmds.size(); i++ ) {
		EditSubCmd sc = mSubCmds[ i ];
		if ( sc.mCmd == SUB_CMD ) {
			if ( sc.mFrom == "" ) {
				CSVTHROW( "Need expression to search for" );
			}
			bool icase = sc.mOpts.find( IC_OPT ) != std::string::npos ;
			ALib::RegEx r( sc.mFrom,
							icase ? ALib::RegEx::Insensitive
							      : ALib::RegEx::Sensitive );
			if ( sc.mOpts.find( ALL_OPT ) != std::string::npos ) {
				r.ReplaceAllIn( f, sc.mTo );
			}
			else {
				r.ReplaceIn( f, sc.mTo );
			}
		}
		else {
			CSVTHROW( "Invalid edit sub-comand: " << sc.mCmd  );
		}
	}
}

//---------------------------------------------------------------------------
// add sub command with some error checking
//---------------------------------------------------------------------------

void EditCommand :: AddSubCmd( const string & ev ) {
	if ( ALib::IsEmpty( ev ) ) {
		CSVTHROW( "Empty value for " << FLAG_EDIT );
	}

	if ( ! isalpha( ev[0] ) ) {			// for now
		CSVTHROW( "Edit sub command missing from " << ev );
	}

	vector <string> tmp;
	if ( ALib::Split( ev, ev[1], tmp ) != 4 ) {
		CSVTHROW( "Invalid value for " << FLAG_EDIT << ": " << ev );
	}

	if ( ev[0] != SUB_CMD ) {
		CSVTHROW( "Invalid edit sub-command '" << ev[0] << "' in '"
						<< ev << "' " );
	}
	mSubCmds.push_back ( EditSubCmd( ev[0], tmp[1], tmp[2], tmp[3] ) );
}

//------------------------------------------------------------------------

} // namespace

// end
