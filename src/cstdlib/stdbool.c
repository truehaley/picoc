/*  */
#include <stdbool.h>

#include "interpreter.h"


static int trueValue = 1;
static int falseValue = 0;


/* structure definitions */
const char StdboolDefs[] = "typedef int bool;";

/* creates various system-dependent definitions */
void StdboolSetupFunc(Picoc *picoc)
{
    /* defines */
    VariableDefinePlatformVar(picoc, NULL, "true", &picoc->IntType,
    	(AnyValue*)&trueValue, false);
    VariableDefinePlatformVar(picoc, NULL, "false", &picoc->IntType,
    	(AnyValue*)&falseValue, false);
    VariableDefinePlatformVar(picoc, NULL, "__bool_true_false_are_defined",
    	&picoc->IntType, (AnyValue*)&trueValue, false);
}
