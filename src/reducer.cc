#include "reducer.h"
#include <errno.h>

using std::endl;


void TopLevel::loadMachineName( string data )
{
	/* Make/get the priority key. The name may have already been referenced
	 * and therefore exist. */
	PriorDictEl *priorDictEl;
	if ( pd->priorDict.insert( data, pd->nextPriorKey, &priorDictEl ) )
		pd->nextPriorKey += 1;
	pd->curDefPriorKey = priorDictEl->value;

	/* Make/get the local error key. */
	LocalErrDictEl *localErrDictEl;
	if ( pd->localErrDict.insert( data, pd->nextLocalErrKey, &localErrDictEl ) )
		pd->nextLocalErrKey += 1;
	pd->curDefLocalErrKey = localErrDictEl->value;
}

void TopLevel::tryMachineDef( InputLoc &loc, std::string name, 
		MachineDef *machineDef, bool isInstance )
{
	GraphDictEl *newEl = pd->graphDict.insert( name );
	if ( newEl != 0 ) {
		/* New element in the dict, all good. */
		newEl->value = new VarDef( name, machineDef );
		newEl->isInstance = isInstance;
		newEl->loc = loc;
		newEl->value->isExport = exportContext[exportContext.length()-1];

		/* It it is an instance, put on the instance list. */
		if ( isInstance )
			pd->instanceList.append( newEl );
	}
	else {
		// Recover by ignoring the duplicate.
		error(loc) << "fsm \"" << name << "\" previously defined" << endl;
	}
}
	
long TopLevel::tryLongScan( const InputLoc &loc, const char *data )
{
	/* Convert the priority number to a long. Check for overflow. */
	long priorityNum;
	errno = 0;

	long aug = strtol( data, 0, 10 );
	if ( errno == ERANGE && aug == LONG_MAX ) {
		/* Priority number too large. Recover by setting the priority to 0. */
		error(loc) << "priority number " << data << 
				" overflows" << endl;
		priorityNum = 0;
	}
	else if ( errno == ERANGE && aug == LONG_MIN ) {
		/* Priority number too large in the neg. Recover by using 0. */
		error(loc) << "priority number " << data << 
				" underflows" << endl;
		priorityNum = 0;
	}
	else {
		/* No overflow or underflow. */
		priorityNum = aug;
	}

	return priorityNum;
}

void TopLevel::reduceString( const char *data )
{
	const char *argv[6];
	argv[0] = "rlparse";
	argv[1] = "string";
	argv[2] = "-";
	argv[3] = id->hostLang->rlhcArg;
	argv[4] = data;
	argv[5] = 0;

	colm_program *program = colm_new_program( &colm_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 5, argv );
	colm_delete_program( program );

}

void TopLevel::reduceFile( const char *inputFileName )
{
	const char *argv[5];
	argv[0] = "rlparse";
	argv[1] = "reduce";
	argv[2] = inputFileName;
	argv[3] = id->hostLang->rlhcArg;
	argv[4] = 0;

	colm_program *program = colm_new_program( &colm_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 4, argv );
	colm_delete_program( program );
}


void SectionPass::reduceFile( const char *inputFileName )
{
	const char *argv[5];
	argv[0] = "rlparse";
	argv[1] = "section";
	argv[2] = inputFileName;
	argv[3] = id->hostLang->rlhcArg;
	argv[4] = 0;

	colm_program *program = colm_new_program( &colm_object );
	colm_set_debug( program, 0 );
	colm_set_reduce_ctx( program, this );
	colm_run_program( program, 4, argv );
	colm_delete_program( program );
}
