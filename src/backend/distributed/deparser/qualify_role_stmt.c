/*-------------------------------------------------------------------------
 *
 * qualify_role_stmt.c
 *	  Functions specialized in fully qualifying all role statements. These
 *	  functions are dispatched from qualify.c
 *
 *	  Fully qualifying role statements consists of adding the current database
 *    name, session user, current use, and current configuration values.
 *
 *	  Goal would be that the deparser functions for these statements can
 *	  serialize the statement without any external lookups.
 *
 * Copyright (c), Citus Data, Inc.
 *
 *-------------------------------------------------------------------------
 */

#include "postgres.h"

#include "distributed/deparser.h"
#include "nodes/parsenodes.h"
#include "nodes/pg_list.h"
#include "utils/builtins.h"
#include "utils/fmgrprotos.h"


static void QualifyVarSetCurrent(VariableSetStmt *setStmt);

/*
 * QualifyAlterRoleSetStmt transforms a
 * ALTER ROLE .. SET ..
 * statement in place and makes the settings fully qualified.
 */
void
QualifyAlterRoleSetStmt(Node *node)
{
	AlterRoleSetStmt *stmt = castNode(AlterRoleSetStmt, node);
	VariableSetStmt *setStmt = stmt->setstmt;

	if (setStmt->kind == VAR_SET_CURRENT)
	{
		QualifyVarSetCurrent(setStmt);
	}
}


/*
 * QualifyVarSetCurrent transforms a
 * FROM CURRENT
 * into a
 * SET config_name TO 'config_value'
 * VariableSetStmt in place and hence makes it fully qualified.
 */
static void
QualifyVarSetCurrent(VariableSetStmt *setStmt)
{
	char *configurationName = setStmt->name;
	text *nameText = cstring_to_text(configurationName);
	Datum nameDatum = PointerGetDatum(nameText);
	Datum configValueDatum = DirectFunctionCall1(show_config_by_name, nameDatum);
	char *configValue = TextDatumGetCString(configValueDatum);

	setStmt->kind = VAR_SET_VALUE;
	setStmt->args = list_make1(MakeSetStatementArgument(configurationName, configValue));
}
