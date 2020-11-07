/*
 * src/tutorial/complex.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"

#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */
#include "access/hash.h"
#include <regex.h>
#include <string.h>
#include "utils/geo_decls.h"
#include "utils/builtins.h"

PG_MODULE_MAGIC;

typedef struct person
{
    int32 length;
    char pname[FLEXIBLE_ARRAY_MEMBER];
} PersonName;

Datum   pname_in(PG_FUNCTION_ARGS);
Datum   pname_out(PG_FUNCTION_ARGS);
Datum   pname_lt(PG_FUNCTION_ARGS);
Datum   pname_le(PG_FUNCTION_ARGS);
Datum   pname_eq(PG_FUNCTION_ARGS);
Datum   pname_not_eq(PG_FUNCTION_ARGS);
Datum   pname_ge(PG_FUNCTION_ARGS);
Datum   pname_gt(PG_FUNCTION_ARGS);
Datum   pname_cmp(PG_FUNCTION_ARGS);
Datum   family(PG_FUNCTION_ARGS);
Datum   given(PG_FUNCTION_ARGS);
Datum   show(PG_FUNCTION_ARGS);
Datum   hash(PG_FUNCTION_ARGS);

void        error_report(char *str);
static int         reg_match(char *str, char *nPattern);
char        *pname_get_part(PersonName *name, int part);
static int  pname_cmp_internal(PersonName * , PersonName *b);


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

PG_FUNCTION_INFO_V1(pname_in);

Datum
pname_in(PG_FUNCTION_ARGS)
{
    char *str = PG_GETARG_CSTRING(0);
    char *namePattern = "^[A-Z]([A-Za-z]|[-]|['])+([ ][A-Z]([A-Za-z]|[-]|['])+)*,[ ]?[[A-Z]([A-Za-z]|[-]|['])+([ ][A-Z]([A-Za-z]|[-]|['])+)*$";
    int index = 0;

    if (strlen(str) < 2)
        error_report(str);
    if (reg_match(str, namePattern)) {
        error_report(str);
    }

    for (index = 0; index < strlen(str); index++) {
        if(str[index] == ',') {
            break;
        }
    }
    if (str[index + 1] == ' '){
        for (index = index + 1; index < strlen(str)-1; index++) {
            str[index] = str[index + 1];
        }
        str[index] = '\0';
    }


    PersonName *result = (PersonName *)palloc(VARHDRSZ + strlen(str) + 1);
    result->length = VARHDRSZ + strlen(str) + 1;
    SET_VARSIZE(result, VARHDRSZ + strlen(str) + 1);
    snprintf(result->pname, strlen(str) + 1, "%s", str);

    PG_RETURN_POINTER(result);
}

void error_report(char *str) {
    ereport(ERROR,
            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                    errmsg("invalid input syntax for type PersonName: \"%s\"",
                           str)));
    return;
}

static int reg_match(char *str, char *nPattern) {
    int result = 1;
    regex_t reg;
    int regInit;

    regInit = regcomp(&reg, nPattern, REG_EXTENDED);
    if (regInit == 0) {
        int ret = regexec(&reg, str, 0, NULL, 0);
        if (ret == 0) {
            result = 0;
        }
    }

    regfree(&reg);

    return result;
}

PG_FUNCTION_INFO_V1(pname_out);

Datum
pname_out(PG_FUNCTION_ARGS)
{
    PersonName *pName = (PersonName *) PG_GETARG_POINTER(0);
    char *result;

    result = psprintf("%s", pName->pname);
    PG_RETURN_CSTRING(result);
}

/*****************************************************************************
 * Operator class for defining B-tree index
 *
 * It's essential that the comparison operators and support function for a
 * B-tree index opclass always agree on the relative ordering of any two
 * data values.  Experience has shown that it's depressingly easy to write
 * unintentionally inconsistent functions.  One way to reduce the odds of
 * making a mistake is to make all the functions simple wrappers around
 * an internal three-way-comparison function, as we do here.
 *****************************************************************************/

/**
 * This is the function to split a PersonName into two parts -- family and given
 * parameter name is the full PersonName
 * index indicates the part of a PersonName it's asking
 * 0 is the family name and 1 is the given name
 **/

char *pname_get_part(PersonName *name, int part){
    int nameIndex = 0;
    char *family;
    char *rawGiven;
    char *given;

    for (nameIndex = 0; nameIndex < strlen(name->pname); nameIndex++) {
        if(name->pname[nameIndex] == ',') {
            break;
        }
    }

    //Family part of pname
    name->pname[nameIndex] = '\0';
    if (part == 0) {
        family = (char *) palloc(nameIndex + 1);
        strcpy(family, name->pname);
        name->pname[nameIndex] = ',';
        return family;
    }

    name->pname[nameIndex] = ',';
    rawGiven = strchr(name->pname, ',');
    // Get rid of comma for given name
    rawGiven++;


    given = (char *) palloc(strlen(name->pname) - nameIndex);
    strcpy(given, rawGiven);

    return given;
}

static int
pname_cmp_internal(PersonName *a, PersonName *b)
{
    char *aFamily = pname_get_part(a,0);
    char *aGiven = pname_get_part(a,1);
    char *bFamily = pname_get_part(b,0);
    char *bGiven = pname_get_part(b,1);

    // A's family name is less than B's family name-->A's pname < B's pname
    if (strcmp(aFamily,bFamily) < 0)
        return -1;
    // A's family name is greater than B's family name-->A's pname > B's pname
    if (strcmp(aFamily,bFamily) > 0)
        return 1;

    // A's family name is equal to B's family name-->look at given names
    if (strcmp(aGiven,bGiven) < 0)
        return -1;
    if (strcmp(aGiven,bGiven) > 0)
        return 1;

    // Both family and given names are equal, return 0
    return 0;

}

PG_FUNCTION_INFO_V1(pname_lt);

Datum
pname_lt(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) < 0);
}

PG_FUNCTION_INFO_V1(pname_le);

Datum
pname_le(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) <= 0);
}

PG_FUNCTION_INFO_V1(pname_eq);

Datum
pname_eq(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);
    PG_RETURN_BOOL(pname_cmp_internal(a, b) == 0);
}

PG_FUNCTION_INFO_V1(pname_not_eq);

Datum
pname_not_eq(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) != 0);
}

PG_FUNCTION_INFO_V1(pname_ge);

Datum
pname_ge(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) >= 0);
}

PG_FUNCTION_INFO_V1(pname_gt);

Datum
pname_gt(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_BOOL(pname_cmp_internal(a, b) > 0);
}

PG_FUNCTION_INFO_V1(pname_cmp);

Datum
pname_cmp(PG_FUNCTION_ARGS)
{
    PersonName *a = (PersonName *) PG_GETARG_POINTER(0);
    PersonName *b = (PersonName *) PG_GETARG_POINTER(1);

    PG_RETURN_INT32(pname_cmp_internal(a, b));
}


/*****************************************************************************
 * Other functions can be used for type PersonName inluding:
 * getting family name of full name,
 * getting given name of full name
 * showing name in display version by appending full family name to the first given name
 *****************************************************************************/

PG_FUNCTION_INFO_V1(family);

Datum
family(PG_FUNCTION_ARGS)
{
    PersonName *full = (PersonName *) PG_GETARG_POINTER(0);

    PG_RETURN_TEXT_P(cstring_to_text(pname_get_part(full,0)));
}

PG_FUNCTION_INFO_V1(given);

Datum
given(PG_FUNCTION_ARGS)
{
    PersonName *full = (PersonName *) PG_GETARG_POINTER(0);

    PG_RETURN_TEXT_P(cstring_to_text(pname_get_part(full,1)));
}

PG_FUNCTION_INFO_V1(show);

Datum
show(PG_FUNCTION_ARGS)
{
    PersonName *full = (PersonName *) PG_GETARG_POINTER(0);
    char *familyName = pname_get_part(full,0);
    char *givenName = pname_get_part(full,1);
    //we only want the first given name, which is the part before space within given name
    int i;
    for (i = 0; i < strlen(givenName); i++) {
        if(givenName[i] == ' ') {
            break;
        }
    }

    givenName[i] = '\0';
    // The length of displayed name is the sum of familyname, firstGiven, plus one for space seperator and one for null terminator
    char *name = (char *)palloc((strlen(familyName) + strlen(givenName) + 2));

    strcpy(name,givenName);
    strcat(name," ");
    strcat(name,familyName);

    PG_RETURN_TEXT_P(cstring_to_text(name));
}

PG_FUNCTION_INFO_V1(hash);

Datum
hash(PG_FUNCTION_ARGS)
{
    PersonName *pName = (PersonName *) PG_GETARG_POINTER(0);
    const char *str = pName->pname;
    int length = strlen(str);

    PG_RETURN_INT32(DatumGetUInt32(hash_any(str, length)));
}


