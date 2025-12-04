/*  */
#include <stdbool.h>

#include "interpreter.h"


static int trueValue = 1;
static int falseValue = 0;


/* structure definitions */
const char StdboolDefs[] = "typedef int bool;";

/* creates various system-dependent definitions */
void StdboolSetupFunc(Picoc *pc)
{
    /* defines */
    VariableDefinePlatformVar(pc, NULL, "true", &pc->IntType,
    	(AnyValue*)&trueValue, false);
    VariableDefinePlatformVar(pc, NULL, "false", &pc->IntType,
    	(AnyValue*)&falseValue, false);
    VariableDefinePlatformVar(pc, NULL, "__bool_true_false_are_defined",
    	&pc->IntType, (AnyValue*)&trueValue, false);
}
