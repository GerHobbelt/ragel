#include "tables.h"
#include "flatvar.h"
#include "binvar.h"

void TablesVar::GOTO( ostream &ret, int gotoDest, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << gotoDest << ";" << CLOSE_GEN_BLOCK();
}

void TablesVar::GOTO_EXPR( ostream &ret, GenInlineItem *ilItem, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << vCS() << " = " << OPEN_HOST_EXPR( "-", 1 );
	INLINE_LIST( ret, ilItem->children, 0, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";" << CLOSE_GEN_BLOCK();
}

void TablesVar::CALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	red->id->error() << "cannot use fcall in -B mode" << std::endl;
	red->id->abortCompile( 1 );
}

void TablesVar::NCALL( ostream &ret, int callDest, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " <<
			vCS() << "; " << TOP() << " += 1;" << vCS() << " = " <<
			callDest << ";" << CLOSE_GEN_BLOCK();
}

void TablesVar::CALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	red->id->error() << "cannot use fcall in -B mode" << std::endl;
	red->id->abortCompile( 1 );
}

void TablesVar::NCALL_EXPR( ostream &ret, GenInlineItem *ilItem, int targState, bool inFinish )
{
	ret << OPEN_GEN_BLOCK();

	if ( red->prePushExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->prePushExpr );
		INLINE_LIST( ret, red->prePushExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << STACK() << "[" << TOP() << "] = " <<
			vCS() << "; " << TOP() << " += 1;" << vCS() <<
			" = " << OPEN_HOST_EXPR( "-", 1 );
	INLINE_LIST( ret, ilItem->children, targState, inFinish, false );
	ret << CLOSE_HOST_EXPR() << ";" << CLOSE_GEN_BLOCK();
}

void TablesVar::RET( ostream &ret, bool inFinish )
{
	red->id->error() << "cannot use fret in -B mode" << std::endl;
	red->id->abortCompile( 1 );
}

void TablesVar::NRET( ostream &ret, bool inFinish )
{
	ret << OPEN_GEN_BLOCK() << TOP() << "-= 1;" << vCS() << " = " <<
			STACK() << "[" << TOP() << "]; ";

	if ( red->postPopExpr != 0 ) {
		ret << OPEN_HOST_BLOCK( red->postPopExpr );
		INLINE_LIST( ret, red->postPopExpr->inlineList, 0, false, false );
		ret << CLOSE_HOST_BLOCK();
	}

	ret << CLOSE_GEN_BLOCK();
}

void TablesVar::BREAK( ostream &ret, int targState, bool csForced )
{
	red->id->error() << "cannot use fbreak in -B mode" << std::endl;
	red->id->abortCompile( 1 );
}

void TablesVar::NBREAK( ostream &ret, int targState, bool csForced )
{
	outLabelUsed = true;
	ret << OPEN_GEN_BLOCK() << P() << "+= 1; _cont = 0; " << CLOSE_GEN_BLOCK();
}

void TablesVar::NFA_POP()
{
	if ( redFsm->anyNfaStates() ) {
		out <<
			"	_nfa_repeat = 1;\n"
			"	while ( _nfa_repeat ) {\n"
			"		_nfa_repeat = 0;\n"
			"	if ( nfa_len > 0 ) {\n"
			"		int _pop_test = 1;\n"
			"		nfa_count += 1;\n"
			"		nfa_len -= 1;\n"
			"		" << P() << " = nfa_bp[nfa_len].p;\n"
			;

		if ( redFsm->bAnyNfaPops ) {
			NFA_FROM_STATE_ACTION_EXEC();

			out << 
				"		switch ( " << ARR_REF( nfaPopTrans ) <<
							"[nfa_bp[nfa_len].popTrans] ) {\n";

			/* Loop the actions. */
			for ( GenActionTableMap::Iter redAct = redFsm->actionMap;
					redAct.lte(); redAct++ )
			{
				if ( redAct->numNfaPopTestRefs > 0 ) {
					/* Write the entry label. */
					out << "\t " << CASE( STR( redAct->actListId+1 ) ) << " {\n";

					/* Write each action in the list of action items. */
					for ( GenActionTable::Iter item = redAct->key; item.lte(); item++ )
						NFA_CONDITION( out, item->value, item.last() );

					out << "\n\t" << CEND() << "}\n";
				}
			}

			out <<
				"		}\n";

			out <<
				"		if ( _pop_test ) {\n"
				"			" << vCS() << " = nfa_bp[nfa_len].state;\n";

			if ( red->nfaPostPopExpr != 0 ) {
				out << OPEN_HOST_BLOCK( red->nfaPostPopExpr );
				INLINE_LIST( out, red->nfaPostPopExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK();
			}

			out <<
//				"			goto _resume;\n"
				"			_nfa_cont = 1;\n"
				"			_nfa_repeat = 0;\n"
				"		}\n";

			if ( red->nfaPostPopExpr != 0 ) {
				out <<
				"			else {\n"
				"			" << OPEN_HOST_BLOCK( red->nfaPostPopExpr );
				INLINE_LIST( out, red->nfaPostPopExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK() << "\n"
//				"				goto _out;\n"
				"				_nfa_cont = 0;\n"
				"				_nfa_repeat = 1;\n"
				"			}\n";
			}
			else {
				out <<
				"			else {\n"
//				"				goto _out;\n"
				"				_nfa_cont = 0;\n"
				"				_nfa_repeat = 1;\n"
				"			}\n"
				;
			}
		}
		else {
			out <<
				"		" << vCS() << " = nfa_bp[nfa_len].state;\n";

			if ( red->nfaPostPopExpr != 0 ) {
				out << OPEN_HOST_BLOCK( red->nfaPostPopExpr );
				INLINE_LIST( out, red->nfaPostPopExpr->inlineList, 0, false, false );
				out << CLOSE_HOST_BLOCK();
			}

			out <<
//				"		goto _resume;\n"
				"		_nfa_cont = 1;\n"
				"		_nfa_repeat = 0;\n"
				;
		}

		out << 
			"	}\n"
			"	else {\n"
			"		_nfa_cont = 0;\n"
			"		_nfa_repeat = 0;\n"
			"	}\n"
			"}\n"
			;
	}
}

void TablesVar::writeExec()
{
	testEofUsed = false;
	outLabelUsed = false;
	matchCondLabelUsed = false;

	if ( redFsm->anyNfaStates() ) {
		out <<
			"{\n"
			"	" << UINT() << " _nfa_cont = 1;\n"
			"	" << UINT() << " _nfa_repeat = 1;\n"
			"	while ( _nfa_cont != 0 )\n";
	}

	out <<
		"	{\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	int _ps;\n";

	out <<
		"	" << UINT() << " _trans = 0;\n"
		"	" << UINT() << " _have = 0;\n"
		"	" << UINT() << " _cont = 1;\n";

	VARS();

	out <<
		"	while ( _cont == 1 ) {\n"
		"\n";

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		_cont = 0;\n";
	}

	out << 
		"_have = 0;\n";

	if ( !noEnd ) {
		out << 
			"	if ( " << P() << " == " << PE() << " ) {\n";

		if ( redFsm->anyEofTrans() || redFsm->anyEofActions() ) {
			out << 
				"	if ( " << P() << " == " << vEOF() << " )\n"
				"	{\n";

			NFA_PUSH( vCS() );

			out << UINT() << " _eofcont = 0;\n";

			out <<
				"	if ( " << ARR_REF( eofCondSpaces ) << "[" << vCS() << "] != -1 ) {\n"
				"		_ckeys = " << OFFSET( ARR_REF( eofCondKeys ),
							/*CAST( UINT() ) + */ ARR_REF( eofCondKeyOffs ) + "[" + vCS() + "]" ) << ";\n"
				"		_klen = " << CAST( "int" ) << ARR_REF( eofCondKeyLens ) + "[" + vCS() + "]" << ";\n"
				"		_cpc = 0;\n"
			;

			if ( red->condSpaceList.length() > 0 )
				COND_EXEC( ARR_REF( eofCondSpaces ) + "[" + vCS() + "]" );

			out <<
				"	{\n"
				"		" << INDEX( ARR_TYPE( eofCondKeys ), "_lower" ) << ";\n"
				"		" << INDEX( ARR_TYPE( eofCondKeys ), "_mid" ) << ";\n"
				"		" << INDEX( ARR_TYPE( eofCondKeys ), "_upper" ) << ";\n"
				"		_lower = _ckeys;\n"
				"		_upper = _ckeys + _klen - 1;\n"
				"		while ( _eofcont == 0 && _lower <= _upper ) {\n"
				"			_mid = _lower + ((_upper-_lower) >> 1);\n"
				"			if ( _cpc < " << CAST( "int" ) << DEREF( ARR_REF( eofCondKeys ), "_mid" ) << " )\n"
				"				_upper = _mid - 1;\n"
				"			else if ( _cpc > " << CAST("int" ) << DEREF( ARR_REF( eofCondKeys ), "_mid" ) << " )\n"
				"				_lower = _mid + 1;\n"
				"			else {\n"
				"				_eofcont = 1;\n"
				"			}\n"
				"		}\n"
				"		if ( _eofcont == 0 ) {\n"
				"			" << vCS() << " = " << ERROR_STATE() << ";\n"
				"		}\n"
				"	}\n"
			;

			out << 
				"	}\n"
				"	else { _eofcont = 1; }\n"
			;

			out << "if ( _eofcont == 1 ) {\n";

			EOF_ACTIONS();

			out << "	}\n";

			out << "if ( _eofcont == 1 ) {\n";

			if ( redFsm->anyEofTrans() ) {
				out <<
					"	if ( " << ARR_REF( eofTrans ) << "[" << vCS() << "] > 0 ) {\n";

				EOF_TRANS();

				out <<
					"		_have = 1;\n"
					"	}\n";

				matchCondLabelUsed = true;
			}

			out << "}\n";
			out << "}\n";
		}

		out << 
			"	if ( _have == 0 )\n"
			"		_cont = 0;\n"
			"	}\n";
	}

	out << 
		"	if ( _cont == 1 ) {\n"
		"	if ( _have == 0 ) {\n";

	FROM_STATE_ACTIONS();

	NFA_PUSH( vCS() );

	LOCATE_TRANS();

	out << "}\n";

	out << "if ( _cont == 1 ) {\n";

	if ( redFsm->anyRegCurStateRef() )
		out << "	_ps = " << vCS() << ";\n";

	string cond = "_cond";
	if ( red->condSpaceList.length() == 0 )
		cond = "_trans";

	out <<
		"	" << vCS() << " = " << CAST("int") << ARR_REF( condTargs ) << "[" << cond << "];\n"
		"\n";

	if ( redFsm->anyRegActions() ) {
		out <<
			"	if ( " << ARR_REF( condActions ) << "[" << cond << "] != 0 ) {\n";

		REG_ACTIONS( cond );

		out <<
			"	}\n";
	}

	TO_STATE_ACTIONS();

	if ( redFsm->errState != 0 ) {
		outLabelUsed = true;
		out << 
			"	if ( " << vCS() << " == " << redFsm->errState->id << " )\n"
			"		_cont = 0;\n";
	}

	out << 
		"	if ( _cont == 1 )\n"
		"		" << P() << " += 1;\n";

	out << "}\n";
	out << "}\n";
	out << "}\n";

	NFA_POP();

	out << "}\n";

	if ( redFsm->anyNfaStates() )
		out << "}\n";
}
