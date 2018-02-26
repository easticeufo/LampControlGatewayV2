/*
    espLib.c -- Embedthis ESP Library Source

    This file is a catenation of all the source code. Amalgamating into a
    single file makes embedding simpler and the resulting application faster.

    Prepared by: orion
 */

#include "esp.h"

#if ME_COM_ESP

/************************************************************************/
/*
    Start of file "src/edi.c"
 */
/************************************************************************/
/*
    edi.c -- Embedded Database Interface (EDI)

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include    "pcre.h"

#if ME_COM_ESP
/************************************* Local **********************************/

static void addValidations();
static void formatFieldForJson(MprBuf *buf, EdiField *fp);
static void manageEdiService(EdiService *es, int flags);
static void manageEdiGrid(EdiGrid *grid, int flags);
static bool validateField(Edi *edi, EdiRec *rec, cchar *columnName, cchar *value);

/************************************* Code ***********************************/

PUBLIC EdiService *ediCreateService()
{
    EdiService      *es;

    if ((es = mprAllocObj(EdiService, manageEdiService)) == 0) {
        return 0;
    }
    MPR->ediService = es;
    es->providers = mprCreateHash(0, MPR_HASH_STATIC_VALUES | MPR_HASH_STABLE);
    addValidations();
    return es;
}


static void manageEdiService(EdiService *es, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(es->providers);
        mprMark(es->validations);
    }
}


PUBLIC int ediAddColumn(Edi *edi, cchar *tableName, cchar *columnName, int type, int flags)
{
    mprRemoveKey(edi->schemaCache, tableName);
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->addColumn(edi, tableName, columnName, type, flags);
}


PUBLIC int ediAddIndex(Edi *edi, cchar *tableName, cchar *columnName, cchar *indexName)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->addIndex(edi, tableName, columnName, indexName);
}


PUBLIC void ediAddProvider(EdiProvider *provider)
{
    EdiService  *es;

    es = MPR->ediService;
    mprAddKey(es->providers, provider->name, provider);
}


static EdiProvider *lookupProvider(cchar *providerName)
{
    EdiService  *es;

    es = MPR->ediService;
    return mprLookupKey(es->providers, providerName);
}


PUBLIC int ediAddTable(Edi *edi, cchar *tableName)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->addTable(edi, tableName);
}


static void manageValidation(EdiValidation *vp, int flags) 
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(vp->name);
        mprMark(vp->data);
    }
}


PUBLIC int ediAddValidation(Edi *edi, cchar *name, cchar *tableName, cchar *columnName, cvoid *data)
{
    EdiService          *es;
    EdiValidation       *prior, *vp; 
    MprList             *validations;
    cchar               *errMsg, *vkey;
    int                 column, next;

    //  FUTURE - should not be attached to "es". Should be unique per database.
    es = MPR->ediService;
    if ((vp = mprAllocObj(EdiValidation, manageValidation)) == 0) {
        return MPR_ERR_MEMORY;
    }
    vp->name = sclone(name);
    if ((vp->vfn = mprLookupKey(es->validations, name)) == 0) {
        mprError("Cannot find validation '%s'", name);
        return MPR_ERR_CANT_FIND;
    }
    if (smatch(name, "format") || smatch(name, "banned")) {
        if (!data || ((char*) data)[0] == '\0') {
            mprError("Bad validation format pattern for %s", name);
            return MPR_ERR_BAD_SYNTAX;
        }
        if ((vp->mdata = pcre_compile2(data, 0, 0, &errMsg, &column, NULL)) == 0) {
            mprError("Cannot compile validation pattern. Error %s at column %d", errMsg, column); 
            return MPR_ERR_BAD_SYNTAX;
        }
        data = 0;
    }
    vp->data = data;
    vkey = sfmt("%s.%s", tableName, columnName);

    lock(edi);
    if ((validations = mprLookupKey(edi->validations, vkey)) == 0) {
        validations = mprCreateList(0, MPR_LIST_STABLE);
        mprAddKey(edi->validations, vkey, validations);
    }
    for (ITERATE_ITEMS(validations, prior, next)) {
        if (prior->vfn == vp->vfn) {
            break;
        }
    }
    if (!prior) {
        mprAddItem(validations, vp);
    }
    unlock(edi);
    return 0;
}


static bool validateField(Edi *edi, EdiRec *rec, cchar *columnName, cchar *value)
{
    EdiValidation   *vp;
    MprList         *validations;
    cchar           *error, *vkey;
    int             next;
    bool            pass;

    assert(edi);
    assert(rec);
    assert(columnName && *columnName);

    pass = 1;
    vkey = sfmt("%s.%s", rec->tableName, columnName);
    if ((validations = mprLookupKey(edi->validations, vkey)) != 0) {
        for (ITERATE_ITEMS(validations, vp, next)) {
            if ((error = (*vp->vfn)(vp, rec, columnName, value)) != 0) {
                if (rec->errors == 0) {
                    rec->errors = mprCreateHash(0, MPR_HASH_STABLE);
                }
                mprAddKey(rec->errors, columnName, sfmt("%s %s", columnName, error));
                pass = 0;
            }
        }
    }
    return pass;
}


PUBLIC int ediChangeColumn(Edi *edi, cchar *tableName, cchar *columnName, int type, int flags)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    mprRemoveKey(edi->schemaCache, tableName);
    return edi->provider->changeColumn(edi, tableName, columnName, type, flags);
}


PUBLIC void ediClose(Edi *edi)
{
    if (!edi || !edi->provider) {
        return;
    }
    edi->provider->close(edi);
}


PUBLIC EdiRec *ediCreateRec(Edi *edi, cchar *tableName)
{
    if (!edi || !edi->provider) {
        return 0;
    }
    return edi->provider->createRec(edi, tableName);
}


PUBLIC int ediDelete(Edi *edi, cchar *path)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->deleteDatabase(path);
}


//  FUTURE - rename edi
PUBLIC void espDumpGrid(EdiGrid *grid)
{
    mprLog(0, "Grid: %s\nschema: %s,\ndata: %s", grid->tableName, ediGetTableSchemaAsJson(grid->edi, grid->tableName),
        ediGridAsJson(grid, MPR_JSON_PRETTY));
}


PUBLIC EdiGrid *ediFilterGridFields(EdiGrid *grid, cchar *fields, int include)
{
    EdiRec      *first, *rec;
    MprList     *fieldList;
    int         f, r, inlist;

    if (!grid || grid->nrecords == 0) {
        return grid;
    }
    first = grid->records[0];
    fieldList = mprCreateListFromWords(fields);

    /* Process list of fields to remove from the grid */
    for (f = 0; f < first->nfields; f++) {
        inlist = mprLookupStringItem(fieldList, first->fields[f].name) >= 0;
        if ((inlist && !include) || (!inlist && include)) {
            for (r = 0; r < grid->nrecords; r++) {
                rec = grid->records[r];
                memmove(&rec->fields[f], &rec->fields[f+1], (rec->nfields - f - 1) * sizeof(EdiField));
                rec->nfields--;
                /* Ensure never saved to database */
                rec->id = 0;
            }
            f--;
        }
    }
    return grid;
}


PUBLIC EdiRec *ediFilterRecFields(EdiRec *rec, cchar *fields, int include)
{
    EdiField    *fp;
    MprList     *fieldList;
    int         inlist;

    if (rec == 0 || rec->nfields == 0) {
        return rec;
    }
    fieldList = mprCreateListFromWords(fields);

    for (fp = rec->fields; fp < &rec->fields[rec->nfields]; fp++) {
        inlist = mprLookupStringItem(fieldList, fp->name) >= 0;
        if ((inlist && !include) || (!inlist && include)) {
            fp[0] = fp[1];
            rec->nfields--;
            fp--;
        }
    }
    /*
        Ensure never saved to database
     */
    rec->id = 0;
    return rec;
}


PUBLIC MprList *ediGetColumns(Edi *edi, cchar *tableName)
{
    if (!edi || !edi->provider) {
        return 0;
    }
    return edi->provider->getColumns(edi, tableName);
}


PUBLIC int ediGetColumnSchema(Edi *edi, cchar *tableName, cchar *columnName, int *type, int *flags, int *cid)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->getColumnSchema(edi, tableName, columnName, type, flags, cid);
}


PUBLIC EdiField *ediGetNextField(EdiRec *rec, EdiField *fp, int offset)
{
    if (rec == 0 || rec->nfields <= 0) {
        return 0;
    }
    if (fp == 0) {
        if (offset >= rec->nfields) {
            return 0;
        }
        fp = &rec->fields[offset];
    } else {
        if (++fp >= &rec->fields[rec->nfields]) {
            fp = 0;  
        }
    }
    return fp;
}


PUBLIC EdiRec *ediGetNextRec(EdiGrid *grid, EdiRec *rec)
{
    int     index;

    if (grid == 0 || grid->nrecords <= 0) {
        return 0;
    }
    if (rec == 0) {
        rec = grid->records[0];
        rec->index = 0;
    } else {
        index = rec->index + 1;
        if (index >= grid->nrecords) {
            rec = 0;  
        } else {
            rec = grid->records[index];
            rec->index = index;
        }
    }
    return rec;
}


PUBLIC cchar *ediGetTableSchemaAsJson(Edi *edi, cchar *tableName)
{
    MprBuf      *buf;
    MprList     *columns;
    cchar       *schema, *s;
    int         c, type, flags, cid, ncols, next;

    if (tableName == 0 || *tableName == '\0') {
        return 0;
    }
    if ((schema = mprLookupKey(edi->schemaCache, tableName)) != 0) {
        return schema;
    }
    buf = mprCreateBuf(0, 0);
    ediGetTableDimensions(edi, tableName, NULL, &ncols);
    columns = ediGetColumns(edi, tableName);
    mprPutStringToBuf(buf, "{\n    \"types\": {\n");
    for (c = 0; c < ncols; c++) {
        ediGetColumnSchema(edi, tableName, mprGetItem(columns, c), &type, &flags, &cid);
        mprPutToBuf(buf, "      \"%s\": {\n        \"type\": \"%s\"\n      },\n", 
            (char*) mprGetItem(columns, c), ediGetTypeString(type));
    }
    if (ncols > 0) {
        mprAdjustBufEnd(buf, -2);
    }
    mprRemoveItemAtPos(columns, 0);
    mprPutStringToBuf(buf, "\n    },\n    \"columns\": [ ");
    for (ITERATE_ITEMS(columns, s, next)) {
        mprPutToBuf(buf, "\"%s\", ", s);
    }
    if (columns->length > 0) {
        mprAdjustBufEnd(buf, -2);
    }
    mprPutStringToBuf(buf, " ]\n  }");
    mprAddNullToBuf(buf);
    schema = mprGetBufStart(buf);
    mprAddKey(edi->schemaCache, tableName, schema);
    return schema;
}


PUBLIC cchar *ediGetGridSchemaAsJson(EdiGrid *grid)
{
    if (grid) {
        return ediGetTableSchemaAsJson(grid->edi, grid->tableName);
    }
    return 0;
}


PUBLIC cchar *ediGetRecSchemaAsJson(EdiRec *rec)
{
    if (rec) {
        return ediGetTableSchemaAsJson(rec->edi, rec->tableName);
    }
    return 0;
}


PUBLIC MprHash *ediGetRecErrors(EdiRec *rec)
{
    return rec->errors;
}


PUBLIC MprList *ediGetGridColumns(EdiGrid *grid)
{
    MprList     *cols;
    EdiRec      *rec;
    EdiField    *fp;

    cols = mprCreateList(0, 0);
    rec = grid->records[0];
    for (fp = rec->fields; fp < &rec->fields[rec->nfields]; fp++) {
        mprAddItem(cols, fp->name);
    }
    return cols;
}


PUBLIC EdiField *ediGetField(EdiRec *rec, cchar *fieldName)
{
    EdiField    *fp;

    for (fp = rec->fields; fp < &rec->fields[rec->nfields]; fp++) {
        if (smatch(fp->name, fieldName)) {
            return fp;
        }
    }
    return 0;
}


PUBLIC cchar *ediGetFieldValue(EdiRec *rec, cchar *fieldName)
{
    EdiField    *fp;

    if (rec == 0) {
        return 0;
    }
    for (fp = rec->fields; fp < &rec->fields[rec->nfields]; fp++) {
        if (smatch(fp->name, fieldName)) {
            return fp->value;
        }
    }
    return 0;
}


PUBLIC int ediGetFieldType(EdiRec *rec, cchar *fieldName)
{
    int     type;
    
    if (ediGetColumnSchema(rec->edi, rec->tableName, fieldName, &type, NULL, NULL) < 0) {
        return 0;
    }
    return type;
}


PUBLIC MprList *ediGetTables(Edi *edi)
{
    if (!edi || !edi->provider) {
        return 0;
    }
    return edi->provider->getTables(edi);
}


PUBLIC int ediGetTableDimensions(Edi *edi, cchar *tableName, int *numRows, int *numCols)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->getTableDimensions(edi, tableName, numRows, numCols);
}


PUBLIC char *ediGetTypeString(int type)
{
    switch (type) {
    case EDI_TYPE_BINARY:
        return "binary";
    case EDI_TYPE_BOOL:
        return "bool";
    case EDI_TYPE_DATE:
        return "date";
    case EDI_TYPE_FLOAT:
        return "float";
    case EDI_TYPE_INT:
        return "int";
    case EDI_TYPE_STRING:
        return "string";
    case EDI_TYPE_TEXT:
        return "text";
    }
    return 0;
}


PUBLIC cchar *ediGridAsJson(EdiGrid *grid, int flags)
{
    EdiRec      *rec;
    EdiField    *fp;
    MprBuf      *buf;
    bool        pretty;
    int         r, f;

    pretty = flags & MPR_JSON_PRETTY;
    buf = mprCreateBuf(0, 0);
    mprPutStringToBuf(buf, "[");
    if (grid) {
        if (pretty) mprPutCharToBuf(buf, '\n');
        for (r = 0; r < grid->nrecords; r++) {
            if (pretty) mprPutStringToBuf(buf, "    ");
            mprPutStringToBuf(buf, "{");
            rec = grid->records[r];
            for (f = 0; f < rec->nfields; f++) {
                fp = &rec->fields[f];
                mprPutToBuf(buf, "\"%s\"", fp->name);
                if (pretty) {
                    mprPutStringToBuf(buf, ": ");
                } else {
                    mprPutCharToBuf(buf, ':');
                }
                formatFieldForJson(buf, fp);
                if ((f+1) < rec->nfields) {
                    mprPutStringToBuf(buf, ",");
                }
            }
            mprPutStringToBuf(buf, "}");
            if ((r+1) < grid->nrecords) {
                mprPutCharToBuf(buf, ',');
            }
            if (pretty) mprPutCharToBuf(buf, '\n');
        }
    }
    mprPutStringToBuf(buf, "]");
    if (pretty) mprPutCharToBuf(buf, '\n');
    mprAddNullToBuf(buf);
    return mprGetBufStart(buf);
}


PUBLIC int ediLoad(Edi *edi, cchar *path)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->load(edi, path);
}


PUBLIC int ediLookupField(Edi *edi, cchar *tableName, cchar *fieldName)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->lookupField(edi, tableName, fieldName);
}


PUBLIC EdiProvider *ediLookupProvider(cchar *providerName)
{
    return lookupProvider(providerName);
}


PUBLIC Edi *ediOpen(cchar *path, cchar *providerName, int flags)
{
    EdiProvider     *provider;

    if ((provider = lookupProvider(providerName)) == 0) {
        mprError("Cannot find EDI provider '%s'", providerName);
        return 0;
    }
    return provider->open(path, flags);
}


PUBLIC Edi *ediClone(Edi *edi)
{
    Edi     *cloned;

    if (!edi || !edi->provider) {
        return 0;
    }
    if ((cloned = edi->provider->open(edi->path, edi->flags)) != 0) {
        cloned->validations = edi->validations;
    }
    return cloned;
}


PUBLIC EdiGrid *ediQuery(Edi *edi, cchar *cmd, int argc, cchar **argv, va_list vargs)
{
    if (!edi || !edi->provider) {
        return 0;
    }
    return edi->provider->query(edi, cmd, argc, argv, vargs);
}


PUBLIC cchar *ediReadFieldValue(Edi *edi, cchar *fmt, cchar *tableName, cchar *key, cchar *columnName, cchar *defaultValue)
{
    EdiField    field;

    field = ediReadField(edi, tableName, key, columnName);
    if (!field.valid) {
        return defaultValue;
    }
    return field.value;
}


PUBLIC EdiRec *ediReadRecWhere(Edi *edi, cchar *tableName, cchar *fieldName, cchar *operation, cchar *value)
{
    EdiGrid *grid;
    
    /* OPT - slow to read entire table. Need optimized query in providers */
    if ((grid = ediReadWhere(edi, tableName, fieldName, operation, value)) == 0) {
        return 0;
    }
    if (grid->nrecords > 0) {
        return grid->records[0];
    }
    return 0;
}


PUBLIC EdiField ediReadField(Edi *edi, cchar *tableName, cchar *key, cchar *fieldName)
{
    if (!edi || !edi->provider) {
        EdiField    field;
        memset(&field, 0, sizeof(EdiField));
        return field;
    }
    return edi->provider->readField(edi, tableName, key, fieldName);
}


PUBLIC EdiRec *ediReadRec(Edi *edi, cchar *tableName, cchar *key)
{
    if (!edi || !edi->provider) {
        return 0;
    }
    return edi->provider->readRec(edi, tableName, key);
}


PUBLIC EdiGrid *ediReadWhere(Edi *edi, cchar *tableName, cchar *fieldName, cchar *operation, cchar *value)
{
    if (!edi || !edi->provider) {
        return 0;
    }
    return edi->provider->readWhere(edi, tableName, fieldName, operation, value);
}


PUBLIC EdiGrid *ediReadTable(Edi *edi, cchar *tableName)
{
    if (!edi || !edi->provider) {
        return 0;
    }
    return edi->provider->readWhere(edi, tableName, 0, 0, 0);
}


PUBLIC cchar *ediRecAsJson(EdiRec *rec, int flags)
{
    MprBuf      *buf;
    EdiField    *fp;
    bool        pretty;
    int         f;

    pretty = flags & MPR_JSON_PRETTY;
    buf = mprCreateBuf(0, 0);
    mprPutStringToBuf(buf, "{ ");
    if (rec) {
        for (f = 0; f < rec->nfields; f++) {
            fp = &rec->fields[f];
            mprPutToBuf(buf, "\"%s\"", fp->name);
            if (pretty) {
                mprPutStringToBuf(buf, ": ");
            } else {
                mprPutCharToBuf(buf, ':');
            }
            formatFieldForJson(buf, fp);
            if ((f+1) < rec->nfields) {
                mprPutStringToBuf(buf, ",");
            }
        }
    }
    mprPutStringToBuf(buf, "}");
    if (pretty) mprPutCharToBuf(buf, '\n');
    mprAddNullToBuf(buf);
    return mprGetBufStart(buf);;
}


PUBLIC int edRemoveColumn(Edi *edi, cchar *tableName, cchar *columnName)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    mprRemoveKey(edi->schemaCache, tableName);
    return edi->provider->removeColumn(edi, tableName, columnName);
}


PUBLIC int ediRemoveIndex(Edi *edi, cchar *tableName, cchar *indexName)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->removeIndex(edi, tableName, indexName);
}


PUBLIC int ediRemoveRec(Edi *edi, cchar *tableName, cchar *key)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->removeRec(edi, tableName, key);
}


PUBLIC int ediRemoveTable(Edi *edi, cchar *tableName)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->removeTable(edi, tableName);
}


PUBLIC int ediRenameTable(Edi *edi, cchar *tableName, cchar *newTableName)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    mprRemoveKey(edi->schemaCache, tableName);
    return edi->provider->renameTable(edi, tableName, newTableName);
}


PUBLIC int ediRenameColumn(Edi *edi, cchar *tableName, cchar *columnName, cchar *newColumnName)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    mprRemoveKey(edi->schemaCache, tableName);
    return edi->provider->renameColumn(edi, tableName, columnName, newColumnName);
}


PUBLIC int ediSave(Edi *edi)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->save(edi);
}


PUBLIC int ediUpdateField(Edi *edi, cchar *tableName, cchar *key, cchar *fieldName, cchar *value)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->updateField(edi, tableName, key, fieldName, value);
}


PUBLIC int ediUpdateRec(Edi *edi, EdiRec *rec)
{
    if (!edi || !edi->provider) {
        return MPR_ERR_BAD_STATE;
    }
    return edi->provider->updateRec(edi, rec);
}


PUBLIC bool ediValidateRec(EdiRec *rec)
{
    EdiField    *fp;
    bool        pass;
    int         c;

    assert(rec->edi);
    if (rec == 0 || rec->edi == 0) {
        return 0;
    }
    pass = 1;
    for (c = 0; c < rec->nfields; c++) {
        fp = &rec->fields[c];
        if (!validateField(rec->edi, rec, fp->name, fp->value)) {
            pass = 0;
            /* Keep going */
        }
    }
    return pass;
}


/********************************* Convenience *****************************/

/*
    Create a free-standing grid. Not saved to the database
    The edi and tableName parameters can be null
 */
PUBLIC EdiGrid *ediCreateBareGrid(Edi *edi, cchar *tableName, int nrows)
{
    EdiGrid  *grid;

    if ((grid = mprAllocMem(sizeof(EdiGrid) + sizeof(EdiRec*) * nrows, MPR_ALLOC_MANAGER | MPR_ALLOC_ZERO)) == 0) {
        return 0;
    }
    mprSetManager(grid, (MprManager) manageEdiGrid);
    grid->nrecords = nrows;
    grid->edi = edi;
    grid->tableName = tableName? sclone(tableName) : 0;
    return grid;
}


/*
    Create a free-standing record. Not saved to the database.
    The tableName parameter can be null. The fields are not initialized (no schema).
 */
PUBLIC EdiRec *ediCreateBareRec(Edi *edi, cchar *tableName, int nfields)
{
    EdiRec      *rec;

    if ((rec = mprAllocMem(sizeof(EdiRec) + sizeof(EdiField) * nfields, MPR_ALLOC_MANAGER | MPR_ALLOC_ZERO)) == 0) {
        return 0;
    }
    mprSetManager(rec, (MprManager) ediManageEdiRec);
    rec->edi = edi;
    rec->tableName = sclone(tableName);
    rec->nfields = nfields;
    return rec;
}


PUBLIC cchar *ediFormatField(cchar *fmt, EdiField *fp)
{
    MprTime     when;

    if (fp->value == 0) {
        return "null";
    }
    switch (fp->type) {
    case EDI_TYPE_BINARY:
    case EDI_TYPE_BOOL:
        return fp->value;

    case EDI_TYPE_DATE:
        if (fmt == 0) {
            fmt = MPR_DEFAULT_DATE;
        }
        if (mprParseTime(&when, fp->value, MPR_UTC_TIMEZONE, 0) == 0) {
            return mprFormatLocalTime(fmt, when);
        }
        return fp->value;

    case EDI_TYPE_FLOAT:
        if (fmt == 0) {
            return fp->value;
        }
        return sfmt(fmt, atof(fp->value));

    case EDI_TYPE_INT:
        if (fmt == 0) {
            return fp->value;
        }
        return sfmt(fmt, stoi(fp->value));

    case EDI_TYPE_STRING:
    case EDI_TYPE_TEXT:
        if (fmt == 0) {
            return fp->value;
        }
        return sfmt(fmt, fp->value);

    default:
        mprError("Unknown field type %d", fp->type);
    }
    return 0;
}


static void formatFieldForJson(MprBuf *buf, EdiField *fp)
{
    MprTime     when;
    cchar       *value;

    value = fp->value;

    if (value == 0) {
        mprPutStringToBuf(buf, "null");
        return;
    } 
    switch (fp->type) {
    case EDI_TYPE_BINARY:
        mprPutToBuf(buf, "-binary-");
        return;

    case EDI_TYPE_STRING:
    case EDI_TYPE_TEXT:
        mprPutToBuf(buf, "\"%s\"", fp->value);
        return;

    case EDI_TYPE_BOOL:
    case EDI_TYPE_FLOAT:
    case EDI_TYPE_INT:
        mprPutStringToBuf(buf, fp->value);
        return;

    case EDI_TYPE_DATE:
        if (mprParseTime(&when, fp->value, MPR_UTC_TIMEZONE, 0) == 0) {
            mprPutToBuf(buf, "\"%s\"", mprFormatUniversalTime(MPR_RFC822_DATE, when));
        } else {
            mprPutToBuf(buf, "\"%s\"", value);
        }
        return;

    default:
        mprError("Unknown field type %d", fp->type);
        mprPutStringToBuf(buf, "null");
    }
}


typedef struct Col {
    EdiGrid     *grid;          /* Source grid for this column */
    EdiField    *fp;            /* Field definition for this column */
    int         joinField;      /* Foreign key field index */
    int         field;          /* Field index in the foreign table */
} Col;


/*
    Create a list of columns to use for a joined table
    For all foreign key columns (ends with "Id"), join the columns from the referenced table.
 */
static MprList *joinColumns(MprList *cols, EdiGrid *grid, MprHash *grids, int joinField, int follow)
{
    EdiGrid     *foreignGrid;
    EdiRec      *rec;
    EdiField    *fp;
    Col         *col;
    cchar       *tableName;
    
    if (grid->nrecords == 0) {
        return cols;
    }
    rec = grid->records[0];
    for (fp = rec->fields; fp < &rec->fields[rec->nfields]; fp++) {
#if KEEP
        if (fp->flags & EDI_FOREIGN && follow)
#else
        if (sends(fp->name, "Id") && follow) 
#endif
        {
            tableName = strim(fp->name, "Id", MPR_TRIM_END);
            if (!(foreignGrid = mprLookupKey(grids, tableName))) {
                col = mprAllocObj(Col, 0);
                col->grid = grid;
                col->field = (int) (fp - rec->fields);
                col->joinField = joinField;
                col->fp = fp;
                mprAddItem(cols, col);
            } else {
                cols = joinColumns(cols, foreignGrid, grids, (int) (fp - rec->fields), 0);
            }
        } else {
#if 0
            if (fp->flags & EDI_KEY && joinField >= 0) {
                /* Don't include ID fields from joined tables */
                continue;
            }
#endif
            col = mprAllocObj(Col, 0);
            col->grid = grid;
            col->field = (int) (fp - rec->fields);
            col->joinField = joinField;
            col->fp = fp;
            mprAddItem(cols, col);
        }
    }
    return cols;
}


/*
    Join grids using an INNER JOIN. All rows are returned.
    List of grids to join must be null terminated.
 */
PUBLIC EdiGrid *ediJoin(Edi *edi, ...)
{
    EdiGrid     *primary, *grid, *result, *current;
    EdiRec      *rec;
    EdiField    *dest, *fp;
    MprList     *cols, *rows;
    MprHash     *grids;
    Col         *col;
    va_list     vgrids;
    cchar       *keyValue;
    int         r, next, nfields, nrows;

    va_start(vgrids, edi);
    if ((primary = va_arg(vgrids, EdiGrid*)) == 0) {
        return 0;
    }
    if (primary->nrecords == 0) {
        return ediCreateBareGrid(edi, NULL, 0);
    }
    /*
        Build list of grids to join
     */
    grids = mprCreateHash(0, MPR_HASH_STABLE);
    for (;;) {
        if ((grid = va_arg(vgrids, EdiGrid*)) == 0) {
            break;
        }
        mprAddKey(grids, grid->tableName, grid);
    }
    va_end(vgrids);

    /*
        Get list of columns for the result. Each col object records the target grid and field index.
     */
    cols = joinColumns(mprCreateList(0, 0), primary, grids, -1, 1);
    nfields = mprGetListLength(cols);
    rows = mprCreateList(0, 0);

    for (r = 0; r < primary->nrecords; r++) {
        if ((rec = ediCreateBareRec(edi, NULL, nfields)) == 0) {
            assert(0);
            return 0;
        }
        mprAddItem(rows, rec);
        dest = rec->fields;
        current = 0;
        for (ITERATE_ITEMS(cols, col, next)) { 
            if (col->grid == primary) {
                *dest = primary->records[r]->fields[col->field];
            } else {
                if (col->grid != current) {
                    current = col->grid;
                    keyValue = primary->records[r]->fields[col->joinField].value;
                    rec = ediReadRecWhere(edi, col->grid->tableName, "id", "==", keyValue);
                }
                if (rec) {
                    fp = &rec->fields[col->field];
                    *dest = *fp;
                    dest->name = sfmt("%s.%s", col->grid->tableName, fp->name);
                } else {
                    dest->name = sclone("UNKNOWN");
                }
            }
            dest++;
        }
    }
    nrows = mprGetListLength(rows);
    if ((result = ediCreateBareGrid(edi, NULL, nrows)) == 0) {
        return 0;
    }
    for (r = 0; r < nrows; r++) {
        result->records[r] = mprGetItem(rows, r);
    }
    result->nrecords = nrows;
    return result;
}


PUBLIC void ediManageEdiRec(EdiRec *rec, int flags)
{
    int     fid;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(rec->edi);
        mprMark(rec->errors);
        mprMark(rec->tableName);
        mprMark(rec->id);

        for (fid = 0; fid < rec->nfields; fid++) {
            mprMark(rec->fields[fid].value);
            mprMark(rec->fields[fid].name);
        }
    }
}


static void manageEdiGrid(EdiGrid *grid, int flags)
{
    int     r;

    if (flags & MPR_MANAGE_MARK) {
        mprMark(grid->edi);
        mprMark(grid->tableName);
        for (r = 0; r < grid->nrecords; r++) {
            mprMark(grid->records[r]);
        }
    }
}


/*
    grid = ediMakeGrid("[ \
        { id: '1', country: 'Australia' }, \
        { id: '2', country: 'China' }, \
    ]");
 */
PUBLIC EdiGrid *ediMakeGrid(cchar *json)
{
    MprJson     *obj, *row, *rp, *cp;
    EdiGrid     *grid;
    EdiRec      *rec;
    EdiField    *fp;
    int         col, r, nrows, nfields;

    if ((obj = mprParseJson(json)) == 0) {
        assert(0);
        return 0;
    }
    nrows = (int) mprGetJsonLength(obj);
    if ((grid = ediCreateBareGrid(NULL, "", nrows)) == 0) {
        assert(0);
        return 0;
    }
    if (nrows <= 0) {
        return grid;
    }
    for (ITERATE_JSON(obj, rp, r)) {
        if (rp->type == MPR_JSON_VALUE) {
            nfields = 1;
            if ((rec = ediCreateBareRec(NULL, "", nfields)) == 0) {
                return 0;
            }
            fp = rec->fields;
            fp->valid = 1;
            fp->name = sclone("value");
            fp->value = rp->value;
            fp->type = EDI_TYPE_STRING;
            fp->flags = 0;
        } else {
            row = rp;
            nfields = (int) mprGetJsonLength(row);
            if ((rec = ediCreateBareRec(NULL, "", nfields)) == 0) {
                return 0;
            }
            fp = rec->fields;
            for (ITERATE_JSON(row, cp, col)) {
                if (fp >= &rec->fields[nfields]) {
                    break;
                }
                fp->valid = 1;
                fp->name = cp->name;
                fp->type = EDI_TYPE_STRING;
                fp->flags = 0;
                fp++;
            }
            if (ediSetFields(rec, row) == 0) {
                assert(0);
                return 0;
            }
        }
        grid->records[r] = rec;
    }
    return grid;
}


PUBLIC MprHash *ediMakeHash(cchar *fmt, ...)
{
    MprHash     *obj;
    va_list     args;

    va_start(args, fmt);
    obj = mprDeserialize(sfmtv(fmt, args));
    va_end(args);
    return obj;
}


PUBLIC MprJson *ediMakeJson(cchar *fmt, ...)
{
    MprJson     *obj;
    va_list     args;

    va_start(args, fmt);
    obj = mprParseJson(sfmtv(fmt, args));
    va_end(args);
    return obj;
}


/*
    rec = ediMakeRec("{ id: 1, title: 'Message One', body: 'Line one' }");
 */
PUBLIC EdiRec *ediMakeRec(cchar *json)
{
    MprHash     *obj;
    MprKey      *kp;
    EdiRec      *rec;
    EdiField    *fp;
    int         f, nfields;

    if ((obj = mprDeserialize(json)) == 0) {
        return 0;
    }
    nfields = mprGetHashLength(obj);
    if ((rec = ediCreateBareRec(NULL, "", nfields)) == 0) {
        return 0;
    }
    for (f = 0, ITERATE_KEYS(obj, kp)) {
        if (kp->type == MPR_JSON_ARRAY || kp->type == MPR_JSON_OBJ) {
            continue;
        }
        fp = &rec->fields[f++];
        fp->valid = 1;
        fp->name = kp->key;
        fp->value = kp->data;
        fp->type = EDI_TYPE_STRING;
        fp->flags = 0;
    }
    return rec;
}


PUBLIC int ediParseTypeString(cchar *type)
{
    if (smatch(type, "binary")) {
        return EDI_TYPE_BINARY;
    } else if (smatch(type, "bool") || smatch(type, "boolean")) {
        return EDI_TYPE_BOOL;
    } else if (smatch(type, "date")) {
        return EDI_TYPE_DATE;
    } else if (smatch(type, "float") || smatch(type, "double") || smatch(type, "number")) {
        return EDI_TYPE_FLOAT;
    } else if (smatch(type, "int") || smatch(type, "integer") || smatch(type, "fixed")) {
        return EDI_TYPE_INT;
    } else if (smatch(type, "string")) {
        return EDI_TYPE_STRING;
    } else if (smatch(type, "text")) {
        return EDI_TYPE_TEXT;
    } else {
        return MPR_ERR_BAD_ARGS;
    }
}


/*
    Swap rows for columns. The key field for each record is set to the prior column name.
 */
PUBLIC EdiGrid *ediPivotGrid(EdiGrid *grid, int flags)
{
    EdiGrid     *result;
    EdiRec      *rec, *first;
    EdiField    *src, *fp;
    int         r, c, nfields, nrows;

    if (grid->nrecords == 0) {
        return grid;
    }
    first = grid->records[0];
    nrows = first->nfields;
    nfields = grid->nrecords;
    result = ediCreateBareGrid(grid->edi, grid->tableName, nrows);
    
    for (c = 0; c < nrows; c++) {
        result->records[c] = rec = ediCreateBareRec(grid->edi, grid->tableName, nfields);
        fp = rec->fields;
        rec->id = first->fields[c].name;
        for (r = 0; r < grid->nrecords; r++) {
            src = &grid->records[r]->fields[c];
            fp->valid = 1;
            fp->name = src->name;
            fp->type = src->type;
            fp->value = src->value;
            fp->flags = src->flags;
            fp++; src++;
        }
    }
    return result;
}

PUBLIC EdiGrid *ediCloneGrid(EdiGrid *grid)
{
    EdiGrid     *result;
    EdiRec      *rec;
    EdiField    *src, *dest;
    int         r, c;

    if (grid->nrecords == 0) {
        return grid;
    }
    result = ediCreateBareGrid(grid->edi, grid->tableName, grid->nrecords);
    for (r = 0; r < grid->nrecords; r++) {
        rec = ediCreateBareRec(grid->edi, grid->tableName, grid->records[r]->nfields);
        result->records[r] = rec;
        rec->id = grid->records[r]->id;
        src = grid->records[r]->fields;
        dest = rec->fields;
        for (c = 0; c < rec->nfields; c++) {
            dest->valid = 1;
            dest->name = src->name;
            dest->value = src->value;
            dest->type = src->type;
            dest->flags = src->flags;
            dest++; src++;
        }
    }
    return result;
}


static cchar *mapEdiValue(cchar *value, int type)
{
    MprTime     time;

    if (value == 0) {
        return value;
    }
    switch (type) {
    case EDI_TYPE_DATE:
        if (!snumber(value)) {
            mprParseTime(&time, value, MPR_UTC_TIMEZONE, 0);
            value = itos(time);
        }
        break;

    case EDI_TYPE_BINARY:
    case EDI_TYPE_BOOL:
    case EDI_TYPE_FLOAT:
    case EDI_TYPE_INT:
    case EDI_TYPE_STRING:
    case EDI_TYPE_TEXT:
    default:
        break;
    }
    return sclone(value);
}


PUBLIC EdiRec *ediSetField(EdiRec *rec, cchar *fieldName, cchar *value)
{
    EdiField    *fp;

    if (rec == 0) {
        return 0;
    }
    if (fieldName == 0) {
        return 0;
    }
    for (fp = rec->fields; fp < &rec->fields[rec->nfields]; fp++) {
        if (smatch(fp->name, fieldName)) {
            fp->value = mapEdiValue(value, fp->type);
            return rec;
        }
    }
    return rec;
}


PUBLIC EdiRec *ediSetFields(EdiRec *rec, MprJson *params)
{
    MprJson     *param;
    int         i;

    if (rec == 0) {
        return 0;
    }
    for (ITERATE_JSON(params, param, i)) {
        if (param->type & MPR_JSON_VALUE) {
            if (!ediSetField(rec, param->name, param->value)) {
                return 0;
            }
        }
    }
    return rec;
}


typedef struct GridSort {
    int     sortColumn;         /**< Column to sort on */
    int     sortOrder;          /**< Sort order: ascending == 1, descending == -1 */
} GridSort;

static int sortRec(EdiRec **r1, EdiRec **r2, GridSort *gs)
{
    EdiField    *f1, *f2;

    f1 = &(*r1)->fields[gs->sortColumn];
    f2 = &(*r2)->fields[gs->sortColumn];
    return scmp(f1->value, f2->value) * gs->sortOrder;
}


static int lookupGridField(EdiGrid *grid, cchar *name)
{
    EdiRec      *rec;
    EdiField    *fp;

    if (grid->nrecords == 0) {
        return MPR_ERR_CANT_FIND;
    }
    rec = grid->records[0];
    for (fp = rec->fields; fp < &rec->fields[rec->nfields]; fp++) {
        if (smatch(name, fp->name)) {
            return (int) (fp - rec->fields);
        }
    }
    return MPR_ERR_CANT_FIND;
}


PUBLIC EdiGrid *ediSortGrid(EdiGrid *grid, cchar *sortColumn, int sortOrder)
{
    GridSort    gs;

    if (grid->nrecords == 0) {
        return grid;
    }
    grid = ediCloneGrid(grid);
    gs.sortColumn = lookupGridField(grid, sortColumn);
    gs.sortOrder = sortOrder;
    mprSort(grid->records, grid->nrecords, sizeof(EdiRec*), (MprSortProc) sortRec, &gs);
    return grid;
}


/********************************* Validations *****************************/

static cchar *checkBoolean(EdiValidation *vp, EdiRec *rec, cchar *fieldName, cchar *value)
{
    if (value && *value) {
        if (scaselessmatch(value, "true") || scaselessmatch(value, "false")) {
            return 0;
        }
    }
    return "is not a number";
}


static cchar *checkDate(EdiValidation *vp, EdiRec *rec, cchar *fieldName, cchar *value)
{
    MprTime     time;

    if (value && *value) {
        if (mprParseTime(&time, value, MPR_UTC_TIMEZONE, NULL) < 0) {
            return 0;
        }
    }
    return "is not a date or time";
}


static cchar *checkFormat(EdiValidation *vp, EdiRec *rec, cchar *fieldName, cchar *value)
{
    int     matched[ME_MAX_ROUTE_MATCHES * 2];

    if (pcre_exec(vp->mdata, NULL, value, (int) slen(value), 0, 0, matched, sizeof(matched) / sizeof(int)) > 0) {
        return 0;
    }
    return "is in the wrong format";
}


static cchar *checkBanned(EdiValidation *vp, EdiRec *rec, cchar *fieldName, cchar *value)
{
    int     matched[ME_MAX_ROUTE_MATCHES * 2];

    if (pcre_exec(vp->mdata, NULL, value, (int) slen(value), 0, 0, matched, sizeof(matched) / sizeof(int)) > 0) {
        return "contains banned content";
    }
    return 0;
}


static cchar *checkInteger(EdiValidation *vp, EdiRec *rec, cchar *fieldName, cchar *value)
{
    if (value && *value) {
        if (snumber(value)) {
            return 0;
        }
    }
    return "is not an integer";
}


static cchar *checkNumber(EdiValidation *vp, EdiRec *rec, cchar *fieldName, cchar *value)
{
    if (value && *value) {
        if (strspn(value, "1234567890+-.") == strlen(value)) {
            return 0;
        }
    }
    return "is not a number";
}


static cchar *checkPresent(EdiValidation *vp, EdiRec *rec, cchar *fieldName, cchar *value)
{
    if (value && *value) {
        return 0;
    }
    return "is missing";
}


static cchar *checkUnique(EdiValidation *vp, EdiRec *rec, cchar *fieldName, cchar *value)
{
    EdiRec  *other;

    //  OPT Could require an index to enforce this.
    if ((other = ediReadRecWhere(rec->edi, rec->tableName, fieldName, "==", value)) == 0) {
        return 0;
    }
    if (smatch(other->id, rec->id)) {
        return 0;
    }
    return "is not unique";
}


PUBLIC void ediAddFieldError(EdiRec *rec, cchar *field, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    if (rec->errors == 0) {
        rec->errors = mprCreateHash(0, MPR_HASH_STABLE);
    }
    mprAddKey(rec->errors, field, sfmtv(fmt, args));
    va_end(args);
}


PUBLIC void ediDefineValidation(cchar *name, EdiValidationProc vfn)
{
    EdiService  *es;

    es = MPR->ediService;
    mprAddKey(es->validations, name, vfn);
}


PUBLIC void ediDefineMigration(Edi *edi, EdiMigration forw, EdiMigration back)
{
    edi->forw = forw;
    edi->back = back;
}


PUBLIC void ediSetPrivate(Edi *edi, bool on)
{
    edi->flags &= ~EDI_PRIVATE;
    if (on) {
        edi->flags |= EDI_PRIVATE;
    }
}


PUBLIC void ediSetReadonly(Edi *edi, bool on)
{
    edi->flags &= ~EDI_NO_SAVE;
    if (on) {
        edi->flags |= EDI_NO_SAVE;
    }
}


static void addValidations()
{
    EdiService  *es;

    es = MPR->ediService;
    /* Thread safe */
    es->validations = mprCreateHash(0, MPR_HASH_STATIC_VALUES);
    ediDefineValidation("boolean", checkBoolean);
    ediDefineValidation("format", checkFormat);
    ediDefineValidation("banned", checkBanned);
    ediDefineValidation("integer", checkInteger);
    ediDefineValidation("number", checkNumber);
    ediDefineValidation("present", checkPresent);
    ediDefineValidation("date", checkDate);
    ediDefineValidation("unique", checkUnique);
}


#endif /* ME_COM_ESP */
/*
    @copy   default

    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */

/************************************************************************/
/*
    Start of file "src/espAbbrev.c"
 */
/************************************************************************/

/*
    espAbbrev.c -- ESP Abbreviated API

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#if ME_COM_ESP

/*************************************** Code *********************************/

PUBLIC void addHeader(cchar *key, cchar *fmt, ...)
{
    va_list     args;
    cchar       *value;

    va_start(args, fmt);
    value = sfmtv(fmt, args);
    espAddHeaderString(getConn(), key, value);
    va_end(args);
}


PUBLIC void addParam(cchar *key, cchar *value)
{
    if (!param(key)) {
        setParam(key, value);
    }
}


PUBLIC bool canUser(cchar *abilities, bool warn)
{
    HttpConn    *conn;

    conn = getConn();
    if (httpCanUser(conn, abilities)) {
        return 1;
    }
    if (warn) {
        setStatus(HTTP_CODE_UNAUTHORIZED);
        sendResult(feedback("error", "Access Denied. Insufficient Privilege."));
    }
    return 0;
}


PUBLIC EdiRec *createRec(cchar *tableName, MprJson *params)
{
    return setRec(ediSetFields(ediCreateRec(getDatabase(), tableName), params));
}


PUBLIC bool createRecFromParams(cchar *table)
{
    return updateRec(createRec(table, params()));
}


/*
    Return the session ID
 */
PUBLIC cchar *createSession()
{
    return espCreateSession(getConn());
}


/*
    Destroy a session and erase the session state data.
    This emits an expired Set-Cookie header to the browser to force it to erase the cookie.
 */
PUBLIC void destroySession()
{
    httpDestroySession(getConn());
}


PUBLIC void dontAutoFinalize()
{
    espSetAutoFinalizing(getConn(), 0);
}


PUBLIC bool feedback(cchar *kind, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    espSetFeedbackv(getConn(), kind, fmt, args);
    va_end(args);
    /*
        Return true if there is not an error feedback message
     */
    return getFeedback("error") == 0;
}


PUBLIC void finalize()
{
    espFinalize(getConn());
}


PUBLIC void flash(cchar *kind, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    espSetFlashv(getConn(), kind, fmt, args);
    va_end(args);
}


PUBLIC void flush()
{
    espFlush(getConn());
}


#if ME_ESP_LEGACY
PUBLIC cchar *getAppUri()
{
    return espGetTop(getConn());
}
#endif


PUBLIC MprList *getColumns(EdiRec *rec)
{
    if (rec == 0) {
        if ((rec = getRec()) == 0) {
            return 0;
        }
    }
    return ediGetColumns(getDatabase(), rec->tableName);
}


PUBLIC HttpConn *getConn()
{
    HttpConn    *conn;

    conn = mprGetThreadData(((Esp*) MPR->espService)->local);
    if (conn == 0) {
        mprError("Connection is not defined in thread local storage.\n"
        "If using a callback, make sure you invoke espSetConn with the connection before using the ESP abbreviated API");
    }
    assert(conn);
    return conn;
}


PUBLIC cchar *getCookies()
{
    return espGetCookies(getConn());
}


PUBLIC MprOff getContentLength()
{
    return espGetContentLength(getConn());
}


PUBLIC cchar *getContentType()
{
    return getConn()->rx->mimeType;
}


PUBLIC void *getData()
{
    return espGetData(getConn());
}


PUBLIC Edi *getDatabase()
{
    return espGetDatabase(getConn());
}


PUBLIC MprDispatcher *getDispatcher()
{
    HttpConn    *conn;

    if ((conn = getConn()) == 0) {
        return 0;
    }
    return conn->dispatcher;
}


PUBLIC cchar *getDocuments()
{
    return getConn()->rx->route->documents;
}


#if ME_ESP_LEGACY
PUBLIC cchar *getDir()
{
    return getDocuments();
}
#endif


PUBLIC EspRoute *getEspRoute()
{
    return espGetEspRoute(getConn());
}


PUBLIC cchar *getFeedback(cchar *kind)
{
    return espGetFeedback(getConn(), kind);
}


PUBLIC cchar *getFlash(cchar *kind)
{
    return espGetFlash(getConn(), kind);
}


PUBLIC cchar *getField(EdiRec *rec, cchar *field)
{
    return ediGetFieldValue(rec, field);
}


PUBLIC cchar *getFieldError(cchar *field)
{
    return mprLookupKey(getRec()->errors, field);
}


PUBLIC EdiGrid *getGrid()
{
    return getConn()->grid;
}


PUBLIC cchar *getHeader(cchar *key)
{
    return espGetHeader(getConn(), key);
}


PUBLIC cchar *getMethod()
{
    return espGetMethod(getConn());
}


PUBLIC cchar *getQuery()
{
    return getConn()->rx->parsedUri->query;
}


PUBLIC EdiRec *getRec()
{
    return getConn()->record;
}


PUBLIC cchar *getReferrer()
{
    return espGetReferrer(getConn());
}


PUBLIC EspReq *getReq()
{
    return getConn()->data;
}


/*
    Get a session and return the session ID. Creates a session if one does not already exist.
 */
PUBLIC cchar *getSessionID()
{
    return espGetSessionID(getConn(), 1);
}


PUBLIC cchar *getSessionVar(cchar *key)
{
    return httpGetSessionVar(getConn(), key, 0);
}


PUBLIC cchar *getConfig(cchar *field)
{
    EspRoute    *eroute;
    cchar       *value;

    eroute = getConn()->rx->route->eroute;
    if ((value = mprGetJson(eroute->config, "value")) == 0) {
        return "";
    }
    return value;
}


#if ME_ESP_LEGACY
PUBLIC cchar *getTop()
{
    return getAppUri();
}
#endif


PUBLIC MprList *getUploads()
{
    return espGetUploads(getConn());
}


PUBLIC cchar *getUri()
{
    return espGetUri(getConn());
}


PUBLIC bool hasGrid()
{
    return espHasGrid(getConn());
}


PUBLIC bool hasRec()
{
    return espHasRec(getConn());
}


PUBLIC bool isEof()
{
    return httpIsEof(getConn());
}


PUBLIC bool isFinalized()
{
    return espIsFinalized(getConn());
}


PUBLIC bool isSecure()
{
    return espIsSecure(getConn());
}


PUBLIC EdiGrid *makeGrid(cchar *contents)
{
    return ediMakeGrid(contents);
}


PUBLIC MprHash *makeHash(cchar *fmt, ...)
{
    va_list     args;
    cchar       *str;

    va_start(args, fmt);
    str = sfmtv(fmt, args);
    va_end(args);
    return mprDeserialize(str);
}


PUBLIC MprJson *makeJson(cchar *fmt, ...)
{
    va_list     args;
    cchar       *str;

    va_start(args, fmt);
    str = sfmtv(fmt, args);
    va_end(args);
    return mprParseJson(str);
}


PUBLIC EdiRec *makeRec(cchar *contents)
{
    return ediMakeRec(contents);
}


PUBLIC cchar *makeUri(cchar *target)
{
    return espUri(getConn(), target);
}


PUBLIC bool modeIs(cchar *kind)
{
    EspRoute    *eroute;

    eroute = getConn()->rx->route->eroute;
    return smatch(eroute->mode, kind);
}


PUBLIC cchar *param(cchar *key)
{
    return espGetParam(getConn(), key, 0);
}


PUBLIC MprJson *params()
{
    return espGetParams(getConn());
}


PUBLIC ssize receive(char *buf, ssize len)
{
    return httpRead(getConn(), buf, len);
}


PUBLIC EdiRec *readRecWhere(cchar *tableName, cchar *fieldName, cchar *operation, cchar *value)
{
    return setRec(ediReadRecWhere(getDatabase(), tableName, fieldName, operation, value));
}


PUBLIC EdiRec *readRec(cchar *tableName, cchar *key)
{
    if (key == 0 || *key == 0) {
        key = "1";
    }
    return setRec(ediReadRec(getDatabase(), tableName, key));
}


PUBLIC EdiRec *readRecByKey(cchar *tableName, cchar *key)
{
    return setRec(ediReadRec(getDatabase(), tableName, key));
}


PUBLIC EdiGrid *readWhere(cchar *tableName, cchar *fieldName, cchar *operation, cchar *value)
{
    return setGrid(ediReadWhere(getDatabase(), tableName, fieldName, operation, value));
}


#if ME_ESP_LEGACY
/* 
    Deprecated in 4.4.1 
 */
PUBLIC EdiGrid *readRecsWhere(cchar *tableName, cchar *fieldName, cchar *operation, cchar *value)
{
    return readWhere(tableName, fieldName, operation, value);
}
#endif


PUBLIC EdiGrid *readTable(cchar *tableName)
{
    return setGrid(ediReadWhere(getDatabase(), tableName, 0, 0, 0));
}


PUBLIC void redirect(cchar *target)
{
    espRedirect(getConn(), 302, target);
}


PUBLIC void redirectBack()
{
    espRedirectBack(getConn());
}


PUBLIC void removeCookie(cchar *name)
{
    espRemoveCookie(getConn(), name);
}


PUBLIC bool removeRec(cchar *tableName, cchar *key)
{
    if (ediRemoveRec(getDatabase(), tableName, key) < 0) {
        feedback("error", "Cannot delete %s", stitle(tableName));
        return 0;
    }
    feedback("inform", "Deleted %s", stitle(tableName));
    return 1;
}


PUBLIC void removeSessionVar(cchar *key)
{
    httpRemoveSessionVar(getConn(), key);
}


PUBLIC ssize render(cchar *fmt, ...)
{
    va_list     args;
    ssize       count;
    cchar       *msg;

    va_start(args, fmt);
    msg = sfmtv(fmt, args);
    count = espRenderString(getConn(), msg);
    va_end(args);
    return count;
}


PUBLIC ssize renderCached()
{
    return espRenderCached(getConn());;
}



PUBLIC ssize renderConfig()
{
    HttpConn    *conn;
    EspRoute    *eroute;

    conn = getConn();
    eroute = conn->rx->route->eroute;
    if (eroute && eroute->client) {
        return renderString(eroute->client);
    }
    return 0;
}


PUBLIC void renderError(int status, cchar *fmt, ...)
{
    va_list     args;
    cchar       *msg;

    va_start(args, fmt);
    msg = sfmt(fmt, args);
    espRenderError(getConn(), status, "%s", msg);
    va_end(args);
}


PUBLIC ssize renderFile(cchar *path)
{
    return espRenderFile(getConn(), path);
}


PUBLIC void renderFlash(cchar *kind)
{
    espRenderFlash(getConn(), kind);
}


PUBLIC ssize renderSafe(cchar *fmt, ...)
{
    va_list     args;
    ssize       count;
    cchar       *msg;

    va_start(args, fmt);
    msg = sfmtv(fmt, args);
    count = espRenderSafeString(getConn(), msg);
    va_end(args);
    return count;
}


PUBLIC ssize renderString(cchar *s)
{
    return espRenderString(getConn(), s);
}


PUBLIC void renderView(cchar *view)
{
    espRenderView(getConn(), view);
}


#if KEEP
PUBLIC int runCmd(cchar *command, char *input, char **output, char **error, MprTime timeout, int flags)
{
    return mprRun(getDispatcher(), command, input, output, error, timeout, MPR_CMD_IN  | MPR_CMD_OUT | MPR_CMD_ERR | flags);
}
#endif


PUBLIC int runCmd(cchar *command, char *input, char **output, char **error, MprTime timeout, int flags)
{
    MprCmd  *cmd;

    cmd = mprCreateCmd(getDispatcher());
    return mprRunCmd(cmd, command, NULL, input, output, error, timeout, MPR_CMD_IN  | MPR_CMD_OUT | MPR_CMD_ERR | flags);
}


/*
    <% scripts(patterns); %>

    Where patterns may contain *, ** and !pattern for exclusion
 */
PUBLIC void scripts(cchar *patterns)
{
    HttpConn    *conn;
    HttpRx      *rx;
    HttpRoute   *route;
    EspRoute    *eroute;
    MprList     *files;
    MprJson     *cscripts, *script;
    cchar       *name, *uri, *path;
    int         next, ci;

    conn = getConn();
    rx = conn->rx;
    route = rx->route;
    eroute = route->eroute;
    patterns = httpExpandRouteVars(route, patterns);

    if (!patterns || !*patterns) {
        if (modeIs("release")) {
            name = sfmt("all-%s.min.js.gz", espGetConfig(route, "version", "1.0.0"));
            scripts(name);
        } else {
            if ((cscripts = mprGetJsonObj(eroute->config, "app.client.scripts")) != 0) {
                for (ITERATE_JSON(cscripts, script, ci)) {
                    scripts(script->value);
                }
            }
        }
        return;
    }
    if ((files = mprGlobPathFiles(eroute->clientDir, patterns, MPR_PATH_RELATIVE)) == 0 || mprGetListLength(files) == 0) {
        mprLog(0, "scripts(): Cannot find any files matching %s", patterns);
    }
    for (ITERATE_ITEMS(files, path, next)) {
        if (schr(path, '$')) {
            path = stemplateJson(path, eroute->config);
        }
        path = sjoin("~/", strim(path, ".gz", MPR_TRIM_END), NULL);
        uri = httpUriToString(httpGetRelativeUri(rx->parsedUri, httpLinkUri(conn, path, 0), 0), 0);
        espRender(conn, "    <script src='%s' type='text/javascript'></script>\n", uri);
    }
}


/*
    Add a security token to the response. The token is generated as a HTTP header and session cookie.
    Note that views will automatically add security tokens to views.
 */
PUBLIC void securityToken()
{
    httpAddSecurityToken(getConn(), 0);
}


PUBLIC ssize sendGrid(EdiGrid *grid)
{
    return espSendGrid(getConn(), grid, 0);
}


PUBLIC ssize sendRec(EdiRec *rec)
{
    return espSendRec(getConn(), rec, 0);
}


PUBLIC void sendResult(bool status)
{
    espSendResult(getConn(), status);
}


PUBLIC void setConn(HttpConn *conn)
{
    espSetConn(conn);
}


PUBLIC void setContentType(cchar *mimeType)
{
    espSetContentType(getConn(), mimeType);
}


PUBLIC void setCookie(cchar *name, cchar *value, cchar *path, cchar *cookieDomain, MprTicks lifespan, bool isSecure)
{
    espSetCookie(getConn(), name, value, path, cookieDomain, lifespan, isSecure);
}


PUBLIC void setData(void *data)
{
    espSetData(getConn(), data);
}


PUBLIC EdiRec *setField(EdiRec *rec, cchar *fieldName, cchar *value)
{
    return ediSetField(rec, fieldName, value);
}


PUBLIC EdiRec *setFields(EdiRec *rec, MprJson *params)
{
    return ediSetFields(rec, params);
}


PUBLIC EdiGrid *setGrid(EdiGrid *grid)
{
    getConn()->grid = grid;
    return grid;
}


PUBLIC void setHeader(cchar *key, cchar *fmt, ...)
{
    va_list     args;
    cchar       *value;

    va_start(args, fmt);
    value = sfmtv(fmt, args);
    espSetHeaderString(getConn(), key, value);
    va_end(args);
}


PUBLIC void setIntParam(cchar *key, int value)
{
    espSetIntParam(getConn(), key, value);
}


PUBLIC void setNotifier(HttpNotifier notifier)
{
    espSetNotifier(getConn(), notifier);
}


PUBLIC void setParam(cchar *key, cchar *value)
{
    espSetParam(getConn(), key, value);
}


PUBLIC EdiRec *setRec(EdiRec *rec)
{
    return espSetRec(getConn(), rec);
}


PUBLIC void setSessionVar(cchar *key, cchar *value)
{
    httpSetSessionVar(getConn(), key, value);
}


PUBLIC void setStatus(int status)
{
    espSetStatus(getConn(), status);
}


PUBLIC cchar *session(cchar *key)
{
    return getSessionVar(key);
}


PUBLIC void setTimeout(void *proc, MprTicks timeout, void *data)
{
    mprCreateEvent(getConn()->dispatcher, "setTimeout", (int) timeout, proc, data, 0);
}


PUBLIC void showRequest()
{
    espShowRequest(getConn());
}


/*
    <% stylesheets(patterns); %>

    Where patterns may contain *, ** and !pattern for exclusion
 */
PUBLIC void stylesheets(cchar *patterns)
{
    HttpConn    *conn;
    HttpRx      *rx;
    HttpRoute   *route;
    EspRoute    *eroute;
    MprList     *files;
    cchar       *uri, *path, *kind;
    int         next;

    conn = getConn();
    rx = conn->rx;
    route = rx->route;
    eroute = route->eroute;
    patterns = httpExpandRouteVars(route, patterns);

    if (!patterns || !*patterns) {
        if (modeIs("release")) {
            stylesheets(sfmt("css/all-%s.min.css", espGetConfig(route, "version", "1.0.0")));
        } else {
            path = mprJoinPath(eroute->clientDir, "css/all.css");
            if (mprPathExists(path, R_OK)) {
                stylesheets("css/all.css");
            } else {
                stylesheets("css/all.less");
                path = mprJoinPath(eroute->clientDir, "css/fix.css");
                if (mprPathExists(path, R_OK)) {
                    stylesheets("css/fix.css");
                }
            }
        }
        return;
    }
    if ((files = mprGlobPathFiles(eroute->clientDir, patterns, MPR_PATH_RELATIVE)) == 0 || mprGetListLength(files) == 0) {
        files = mprCreateList(0, 0);
        mprAddItem(files, patterns);
    }
    for (ITERATE_ITEMS(files, path, next)) {
        path = sjoin("~/", strim(path, ".gz", MPR_TRIM_END), NULL);
        uri = httpUriToString(httpGetRelativeUri(rx->parsedUri, httpLinkUri(conn, path, 0), 0), 0);
        kind = mprGetPathExt(path);
        if (smatch(kind, "css")) {
            espRender(conn, "    <link rel='stylesheet' type='text/css' href='%s' />\n", uri);
        } else {
            espRender(conn, "    <link rel='stylesheet/%s' type='text/css' href='%s' />\n", kind, uri);
        }
    }
}


PUBLIC void updateCache(cchar *uri, cchar *data, int lifesecs)
{
    espUpdateCache(getConn(), uri, data, lifesecs);
}


PUBLIC bool updateField(cchar *tableName, cchar *key, cchar *fieldName, cchar *value)
{
    return ediUpdateField(getDatabase(), tableName, key, fieldName, value) == 0;
}


PUBLIC bool updateFields(cchar *tableName, MprJson *params)
{
    EdiRec  *rec;
    cchar   *key;

    key = mprLookupJson(params, "id");
    if ((rec = ediSetFields(ediReadRec(getDatabase(), tableName, key), params)) == 0) {
        return 0;
    }
    return updateRec(rec);
}


PUBLIC bool updateRec(EdiRec *rec)
{
    if (!rec) {
        feedback("error", "Cannot save record");
        return 0;
    }
    setRec(rec);
    if (ediUpdateRec(getDatabase(), rec) < 0) {
        feedback("error", "Cannot save %s", stitle(rec->tableName));
        return 0;
    }
    feedback("inform", "Saved %s", stitle(rec->tableName));
    return 1;
}


PUBLIC bool updateRecFromParams(cchar *table)
{
    return updateRec(setFields(readRec(table, param("id")), params()));
}


PUBLIC cchar *uri(cchar *target, ...)
{
    va_list     args;
    cchar       *uri;

    va_start(args, target);
    uri = sfmtv(target, args);
    va_end(args);
    return httpLink(getConn(), uri);
}


#endif /* ME_COM_ESP */

/*
    @copy   default

    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */

/************************************************************************/
/*
    Start of file "src/espDeprecated.c"
 */
/************************************************************************/
/*
    espDeprecated.c -- Deprecated HTML controls 

    This code was deprecated in 4.2 and will soon be removed.

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#if (ME_COM_ESP && ME_ESP_LEGACY)
/************************************* Local **********************************/

#define EDATA(s)        "data-esp-" s           /* Prefix for data attributes */
#define ESTYLE(s)       "esp-" s                /* Prefix for ESP styles */

#define SCRIPT_IE       0x1
#define SCRIPT_LESS     0x2

typedef struct EspScript {
    cchar   *name;                              /* Script name */
    cchar   *option;                            /* Esp control option that must be present to trigger emitting script */
    int     flags;                              /* Conditional generation flags */
} EspScript;

static EspScript defaultScripts[] = {
    { "/js/jquery",              0,              0 },
    { "/js/jquery.tablesorter",  "tablesorter",  0 },
    { "/js/jquery.simplemodal",  "simplemodal",  0 },
    { "/js/jquery.esp",          0,              0 },
    { "/js/html5shiv",           0,              SCRIPT_IE },
    { "/js/respond",             "respond",      SCRIPT_IE },
    { "/js/less",                "less",         SCRIPT_LESS },
    { 0,                         0,              0 },
};

static char *defaultLess[] = {
    "/css/all.less", 
    0,
};

static char *defaultCss[] = {
    "/css/all.css", 
    0,
};

static char *internalOptions[] = {
    "action",                       /* Service/Action to invoke */
    "cell",                         /* table(): If set, table clicks apply to the cell and not to the row */
    "click",                        /* general: URI to invoke if the control is clicked */
    "columns",                      /* table(): Column options */
    "controller",                   /* general: Service to use for click events */
    "escape",                       /* general: Html escape the data */
    "feedback",                     /* general: User feedback overlay after a clickable event */
    "field",
    "formatter",                    /* general: Printf style format string to apply to data */
    "header",                       /* table(): Column options header */
    "hidden",                       /* text(): Field should be hidden */
    "hideErrors",                   /* form(): Hide record validation errors */
    "insecure",                     /* form(): Don't generate a CSRF security token */
    "key",                          /* general: key property/value data to provide for clickable events */
    "keyFormat",                    /* General: How keys are handled for click events: "params" | "path" | "query" */
    "kind",
    "minified",                     /* script(): Use minified script variants */
    "name",                         /* table(): Column options name */
    "params",                       /* general: Parms to pass on click events */
    "pass",
    "password",                     /* text(): Text input is a password */
    "pivot",                        /* table(): Pivot the table data */
    "remote",                       /* general: Set to true to make click event operate in the background */
    "retain",
    "securityToken",                /* form(): Name of security token to use */
    "showHeader",                   /* table(): Show table column names header  */
    "showId",                       /* table(): Show the ID column */
    "sort",                         /* table(): Column to sort rows by */
    "sortOrder",                    /* table(): Row sort order */
    "styleCells",                   /* table(): Styles to use for table cells */
    "styleColumns",                 /* table(): Styles to use for table columns */
    "styleRows",                    /* table(): Styles to use for table rows */
    "title",                        /* table(): Table title to display */
    "toggle",                       /* tabs(): Toggle tabbed panes */
    "value",                        /* general: Value to use instead of record-bound data */
    0
};

static cchar *escapeValue(cchar *value, MprHash *options);
static cchar *formatValue(EdiField *fp, MprHash *options);
static cchar *getValue(HttpConn *conn, cchar *field, MprHash *options);
static cchar *map(HttpConn *conn, MprHash *options);
static void textInner(HttpConn *conn, cchar *field, MprHash *options);

/************************************* Code ***********************************/

PUBLIC void espAlert(HttpConn *conn, cchar *text, cchar *optionString)
{
    MprHash     *options;
   
    options = httpGetOptions(optionString);
    httpInsertOption(options, "class", ESTYLE("alert"));
    text = escapeValue(text, options);
    espRender(conn, "<div%s>%s</div>", map(conn, options), text);
}


PUBLIC void espAnchor(HttpConn *conn, cchar *text, cchar *uri, cchar *optionString) 
{
    MprHash     *options;

    options = httpGetOptions(optionString);
    text = escapeValue(text, options);
    espRender(conn, "<a href='%s'%s>%s</a>", uri, map(conn, options), text);
}


PUBLIC void espButton(HttpConn *conn, cchar *name, cchar *value, cchar *optionString)
{
    MprHash     *options;

    options = httpGetOptions(optionString);
    espRender(conn, "<input name='%s' type='submit' value='%s'%s />", name, value, map(conn, options));
}


PUBLIC void espButtonLink(HttpConn *conn, cchar *text, cchar *uri, cchar *optionString)
{
    MprHash     *options;

    options = httpGetOptions(optionString);
    httpSetOption(options, EDATA("click"), httpLink(conn, uri));
    espRender(conn, "<button%s>%s</button>", map(conn, options), text);
}


/*
    checkbox field 

    checkedValue -- Value for which the checkbox will be checked. Defaults to true.
    Example:
        checkbox("admin", "true")
 */
PUBLIC void espCheckbox(HttpConn *conn, cchar *field, cchar *checkedValue, cchar *optionString) 
{
    MprHash     *options;
    cchar       *value, *checked;
   
    options = httpGetOptions(optionString);
    value = getValue(conn, field, options);
    checked = scaselessmatch(value, checkedValue) ? " checked='yes'" : "";
    espRender(conn, "<input name='%s' type='checkbox'%s%s value='%s' />\r\n", field, map(conn, options), 
        checked, checkedValue);
    espRender(conn, "    <input name='%s' type='hidden'%s value='' />", field, map(conn, options));
}


PUBLIC void espDivision(HttpConn *conn, cchar *body, cchar *optionString) 
{
    MprHash     *options;

    options = httpGetOptions(optionString);
    espRender(conn, "<div%s>%s</div>", map(conn, options), body);
}


/*
    dropdown("priority", makeGrid("[{ id: 0, low: 0}, { id: 1, med: 1}, {id: 2, high: 2}]"), 0)
    dropdown("priority", makeGrid("[{ low: 0}, { med: 1}, {high: 2}]"), 0)
    dropdown("priority", makeGrid("[0, 10, 100]"), 0)

    Options can provide the defaultValue in a "value" property.
 */
PUBLIC void espDropdown(HttpConn *conn, cchar *field, EdiGrid *choices, cchar *optionString) 
{
    EdiRec      *rec;
    MprHash     *options;
    cchar       *id, *currentValue, *selected, *value;
    int         r;

    if (choices == 0) {
        return;
    }
    options = httpGetOptions(optionString);
    currentValue = getValue(conn, field, options);
    if (field == 0) {
        field = httpGetOption(options, "field", "id");
    }
    espRender(conn, "<select name='%s'%s>\r\n", field, map(conn, options));
    for (r = 0; r < choices->nrecords; r++) {
        rec = choices->records[r];
        if (rec->nfields == 1) {
            value = rec->fields[0].value;
        } else {
            value = ediGetFieldValue(rec, field);
        }
        if ((id = ediGetFieldValue(choices->records[r], "id")) == 0) {
            id = value;
        }
        selected = (smatch(value, currentValue)) ? " selected='yes'" : "";
        espRender(conn, "        <option value='%s'%s>%s</option>\r\n", id, selected, value);
    }
    espRender(conn, "    </select>");
}


PUBLIC void espIcon(HttpConn *conn, cchar *uri, cchar *optionString)
{
    MprHash     *options;
    EspRoute    *eroute;

    eroute = conn->rx->route->eroute;
    options = httpGetOptions(optionString);
    if (uri == 0) {
        /* Suppress favicon */
        uri = "data:image/x-icon;,";
    } else if (*uri == 0) {
        uri = sjoin("~/", mprGetPathBase(eroute->clientDir), "/images/favicon.ico", NULL);
    }
    espRender(conn, "<link href='%s' rel='shortcut icon'%s />", httpLink(conn, uri), map(conn, options));
}


PUBLIC void espImage(HttpConn *conn, cchar *uri, cchar *optionString)
{
    MprHash     *options;

    options = httpGetOptions(optionString);
    espRender(conn, "<img src='%s'%s />", httpLink(conn, uri), map(conn, options));
}


PUBLIC void espLabel(HttpConn *conn, cchar *text, cchar *optionString)
{
    MprHash     *options;

    options = httpGetOptions(optionString);
    espRender(conn, "<span%s>%s</span>", map(conn, options), text);
}


PUBLIC void espMail(HttpConn *conn, cchar *name, cchar *address, cchar *optionString) 
{
    MprHash     *options;

    options = httpGetOptions(optionString);
    espRender(conn, "<a href='mailto:%s'%s>%s</a>", address, map(conn, options), name);
}


PUBLIC void espProgress(HttpConn *conn, cchar *percent, cchar *optionString)
{
    MprHash     *options;
   
    options = httpGetOptions(optionString);
    httpAddOption(options, EDATA("progress"), percent);
    espRender(conn, "<div class='" ESTYLE("progress") "'>\r\n");
    espRender(conn, "    <div class='" ESTYLE("progress-inner") "'%s>%s %%</div>\r\n</div>", map(conn, options), percent);
}


/*
    radio("priority", "{low: 0, med: 1, high: 2}", NULL)
    radio("priority", "{low: 0, med: 1, high: 2}", "{value:'2'}")
 */
PUBLIC void espRadio(HttpConn *conn, cchar *field, cchar *choicesString, cchar *optionsString)
{
    MprKey      *kp;
    MprHash     *choices, *options;
    cchar       *value, *checked;

    choices = httpGetOptions(choicesString);
    options = httpGetOptions(optionsString);
    value = getValue(conn, field, options);
    for (kp = 0; (kp = mprGetNextKey(choices, kp)) != 0; ) {
        checked = (smatch(kp->data, value)) ? " checked" : "";
        espRender(conn, "%s <input type='radio' name='%s' value='%s'%s%s />\r\n", 
            stitle(kp->key), field, kp->data, checked, map(conn, options));
    }
}


/*
    Control the refresh of dynamic elements in the page
 */
PUBLIC void espRefresh(HttpConn *conn, cchar *on, cchar *off, cchar *optionString)
{
    MprHash     *options;
   
    options = httpGetOptions(optionString);
    espRender(conn, "<img src='%s' " EDATA("on") "='%s' " EDATA("off") "='%s%s' class='" ESTYLE("refresh") "' />", on, on, off, map(conn, options));
}


PUBLIC void espScript(HttpConn *conn, cchar *uri, cchar *optionString)
{
    MprHash     *options;
    cchar       *indent, *newline, *path;
    EspScript   *sp;
    EspRoute    *eroute;
    bool        minified;
   
    eroute = conn->rx->route->eroute;
    options = httpGetOptions(optionString);
    if (uri) {
        espRender(conn, "<script src='%s' type='text/javascript'></script>", httpLink(conn, uri));
    } else {
        minified = smatch(httpGetOption(options, "minified", 0), "true");
        indent = "";
        for (sp = defaultScripts; sp->name; sp++) {
            if (sp->option && !smatch(httpGetOption(options, sp->option, "false"), "true")) {
                continue;
            }
            if (sp->flags & SCRIPT_IE) {
                espRender(conn, "%s<!-- [if lt IE 9]>\n", indent);
            }
            path = sjoin("~/", mprGetPathBase(eroute->clientDir), sp->name, minified ? ".min.js" : ".js", NULL);
            uri = httpLink(conn, path);
            newline = sp[1].name ? "\r\n" :  "";
            espRender(conn, "%s<script src='%s' type='text/javascript'></script>%s", indent, uri, newline);
            if (sp->flags & SCRIPT_IE) {
                espRender(conn, "%s<![endif]-->\n", indent);
            }
            indent = "    ";
        }
    }
}


PUBLIC void espStylesheet(HttpConn *conn, cchar *uri, cchar *optionString) 
{
    EspRoute    *eroute;
    MprHash     *options;
    cchar       *indent, *newline;
    char        **up;
    bool        less;
   
    eroute = conn->rx->route->eroute;
    if (uri) {
        espRender(conn, "<link rel='stylesheet' type='text/css' href='%s' />", httpLink(conn, uri));
    } else {
        indent = "";
        options = httpGetOptions(optionString);
        less = smatch(httpGetOption(options, "type", "css"), "less");
        up = less ? defaultLess : defaultCss;
        for (; *up; up++) {
            uri = httpLink(conn, sjoin("~/", mprGetPathBase(eroute->clientDir), *up, NULL));
            newline = up[1] ? "\r\n" :  "";
            espRender(conn, "%s<link rel='stylesheet%s' type='text/css' href='%s' />%s", indent, less ? "/less" : "", uri, newline);
            indent = "    ";
        }
    }
}


/*
    Grid is modified. Columns are removed and sorted as required.
 */
static void filterCols(EdiGrid *grid, MprHash *options, MprHash *colOptions)
{
    MprList     *gridCols;
    MprHash     *cp;
    EdiRec      *rec;
    EdiField    f;
    cchar       *columnName;
    char        key[8];
    int         ncols, r, fnum, currentPos, c, index, *desired, *location, pos, t;

    gridCols = ediGetGridColumns(grid);

    if (colOptions) {
        /*
            Sort grid record columns into the order specified by the column options
         */
        ncols = grid->records[0]->nfields;
        location = mprAlloc(sizeof(int) * ncols);
        for (c = 0; c < ncols; c++) {
            location[c] = c;
        }
        ncols = mprGetHashLength(colOptions);
        desired = mprAlloc(sizeof(int) * ncols);
        for (c = 0; c < ncols; c++) {
            cp = mprLookupKey(colOptions, itosbuf(key, sizeof(key), c, 10));
            if ((columnName = mprLookupKey(cp, "name")) == 0) {
                mprError("Cannot locate \"name\" field for column in table");
                return;
            }
            pos = mprLookupStringItem(gridCols, columnName);
            if (pos < 0) {
                mprError("Cannot find column \"%s\", columns: %s", columnName, mprListToString(gridCols, ", "));
            } else {
                desired[c] = pos;
                location[c] = c;
            }
        }
        for (c = 0; c < ncols; c++) {
            fnum = c;
            for (r = 0; r < grid->nrecords; r++) {
                rec = grid->records[r];
                rec->nfields = ncols;
                fnum = desired[c];
                if (fnum < 0) {
                    continue;
                }
                currentPos = location[fnum];

                f = rec->fields[c];
                rec->fields[c] = rec->fields[currentPos];
                rec->fields[currentPos] = f;
            }
            t = location[c];
            location[c] = location[fnum];
            location[fnum] = t;
        }
        
    } else {
        /*
            If showId is false, remove the "id" column
         */
        if (httpOption(options, "showId", "false", 0) && (index = mprLookupStringItem(gridCols, "id")) >= 0) {
            for (r = 0; r < grid->nrecords; r++) {
                rec = grid->records[r];
                rec->nfields--;
                mprMemcpy(rec->fields, sizeof(EdiField) * rec->nfields, &rec->fields[index],
                    sizeof(EdiField) * (rec->nfields - 1));
            }
        }
    }
}


static char *hashToString(MprHash *hash, cchar *sep)
{
    MprBuf  *buf;
    cchar   *data;
    char    key[8];
    int     i, len;

    len = mprGetHashLength(hash);
    buf = mprCreateBuf(0, 0);
    mprPutCharToBuf(buf, '{');
    for (i = 0; i < len; ) {
        data = mprLookupKey(hash, itosbuf(key, sizeof(key), i, 10));
        mprPutStringToBuf(buf, data);
        if (++i < len) {
            mprPutStringToBuf(buf, sep ? sep : ",");
        }
    }
    mprPutCharToBuf(buf, '}');
    mprAddNullToBuf(buf);
    return mprGetBufStart(buf);
}


static EdiGrid *hashToGrid(MprHash *hash)
{
    EdiGrid *grid;
    EdiRec  *rec;
    cchar   *data;
    char    key[8];
    int     i, len;

    len = mprGetHashLength(hash);
    grid = ediCreateBareGrid(NULL, "grid", len);
    for (i = 0; i < len; i++) {
        data = mprLookupKey(hash, itosbuf(key, sizeof(key), i, 10));
        grid->records[i] = rec = ediCreateBareRec(NULL, "grid", 1);
        rec->fields[0].name = sclone("value");
        rec->fields[0].type = EDI_TYPE_STRING;
        rec->fields[0].value = data;
    }
    return grid;
}


/*
    Render a table from a data grid

    Examples:

        table(grid, "{ refresh:'@update', period:'1000', pivot:'true' }");
        table(grid, "{ click:'@edit' }");
        table(grid, "columns: [ \
            { name: product, header: 'Product', width: '20%' }, \
            { name: date,    format: '%m-%d-%y' }, \
            { name: 'user.name' }, \
        ]");
        table(readTable("users"));
        table(makeGrid("[{'name': 'peter', age: 23 }, {'name': 'mary', age: 22}]")
        table(grid, "{ \
            columns: [ \
                { name: 'speed',         header: 'Speed', dropdown: [100, 1000, 40000] }, \
                { name: 'adminMode',     header: 'Admin Mode', radio: ['Up', 'Down'] }, \
                { name: 'state',         header: 'State', radio: ['Enabled', 'Disabled'] }, \
                { name: 'autoNegotiate', header: 'Auto Negotiate', checkbox: ['enabled'] }, \
                { name: 'type',          header: 'Type' }, \
            ], \
            edit: true, \
            pivot: true, \
            showHeader: false, \
            class: 'esp-pivot', \
        }");

 */
static void pivotTable(HttpConn *conn, EdiGrid *grid, MprHash *options)
{
    MprHash     *colOptions, *rowOptions, *thisCol, *dropdown, *radio;
    MprList     *cols;
    EdiRec      *rec;
    EdiField    *fp;
    cchar       *title, *width, *o, *header, *name, *checkbox;
    char        index[8];
    int         c, r, ncols;
   
    assert(grid);
    if (grid->nrecords == 0) {
        espRender(conn, "<p>No Data</p>\r\n");
        return;
    }
    colOptions = httpGetOptionHash(options, "columns");
    cols = ediGetGridColumns(grid);
    ncols = mprGetListLength(cols);
    rowOptions = mprCreateHash(0, MPR_HASH_STABLE);
    httpSetOption(rowOptions, EDATA("click"), httpGetOption(options, EDATA("click"), 0));
    httpInsertOption(options, "class", ESTYLE("pivot"));
    httpInsertOption(options, "class", ESTYLE("table"));
    espRender(conn, "<table%s>\r\n", map(conn, options));

    /*
        Table header
     */
    if (httpOption(options, "showHeader", "true", 1)) {
        espRender(conn, "    <thead>\r\n");
        if ((title = httpGetOption(options, "title", 0)) != 0) {
            espRender(conn, "        <tr class='" ESTYLE("table-title") "'><td colspan='%s'>%s</td></tr>\r\n", 
                mprGetListLength(cols), title);
        }
        espRender(conn, "        <tr class='" ESTYLE("header") "'>\r\n");
        rec = grid->records[0];
        for (r = 0; r < ncols; r++) {
            assert(r <= grid->nrecords);
            width = ((o = httpGetOption(options, "width", 0)) != 0) ? sfmt(" width='%s'", o) : "";
            thisCol = mprLookupKey(colOptions, itosbuf(index, sizeof(index), r, 10));
            header = httpGetOption(thisCol, "header", stitle(rec->id));
            espRender(conn, "            <th%s>%s</th>\r\n", width, header);
        }
        espRender(conn, "        </tr>\r\n    </thead>\r\n");
    }
    espRender(conn, "    <tbody>\r\n");

    /*
        Table body data
     */
    for (r = 0; r < grid->nrecords; r++) {
        rec = grid->records[r];
        httpSetOption(rowOptions, "id", rec->id);
        espRender(conn, "        <tr%s>\r\n", map(conn, rowOptions));
        for (c = 0; c < ncols; c++) {
            fp = &rec->fields[c];
            thisCol = mprLookupKey(colOptions, itosbuf(index, sizeof(index), r, 10));
            if (httpGetOption(thisCol, "align", 0) == 0) {
                if (fp->type == EDI_TYPE_INT || fp->type == EDI_TYPE_FLOAT) {
                    if (!thisCol) {
                        thisCol = mprCreateHash(0, MPR_HASH_STABLE);
                    }
                    httpInsertOption(thisCol, "align", "right");
                }
            }
            if (c == 0) {
                /* 
                    Render column name
                 */
                name = httpGetOption(thisCol, "header", stitle(rec->id));
                if (httpOption(options, "edit", "true", 0) && httpOption(thisCol, "edit", "true", 1)) {
                    espRender(conn, "            <td%s>%s</td><td>", map(conn, thisCol), name);
                    if ((dropdown = httpGetOption(thisCol, "dropdown", 0)) != 0) {
                        espDropdown(conn, fp->name, hashToGrid(dropdown), 0);
                    } else if ((radio = httpGetOption(thisCol, "radio", 0)) != 0) {
                        espRadio(conn, fp->name, hashToString(radio, 0), 0);
                    } else if ((checkbox = httpGetOption(thisCol, "checkbox", 0)) != 0) {
                        espCheckbox(conn, fp->name, checkbox, 0);
                    } else {
                        input(fp->name, 0);
                    }
                    espRender(conn, "</td>\r\n");
                } else {
                    espRender(conn, "            <td%s>%s</td><td>%s</td>\r\n", map(conn, thisCol), name, fp->value);
                }                
            } else {
                espRender(conn, "            <td%s>%s</td>\r\n", map(conn, thisCol), fp->value);
            }
        }
    }
    espRender(conn, "        </tr>\r\n");
    espRender(conn, "    </tbody>\r\n</table>\r\n");
}


PUBLIC void espTable(HttpConn *conn, EdiGrid *grid, cchar *optionString)
{
    MprHash     *options, *colOptions, *rowOptions, *thisCol;
    MprList     *cols;
    EdiRec      *rec;
    EdiField    *fp;
    cchar       *title, *width, *o, *header, *value, *sortColumn;
    char        index[8];
    int         c, r, ncols, sortOrder;
   
    assert(grid);
    if (grid == 0) {
        return;
    }
    options = httpGetOptions(optionString);
    if (grid->nrecords == 0) {
        espRender(conn, "<p>No Data</p>\r\n");
        return;
    }
    if (grid->flags & EDI_GRID_READ_ONLY) {
        grid = ediCloneGrid(grid);
    }
    if ((sortColumn = httpGetOption(options, "sort", 0)) != 0) {
        sortOrder = httpOption(options, "sortOrder", "ascending", 1);
        ediSortGrid(grid, sortColumn, sortOrder);
    }
    colOptions = httpGetOptionHash(options, "columns");

    filterCols(grid, options, colOptions);

    if (httpOption(options, "pivot", "true", 0) != 0) {
        pivotTable(conn, ediPivotGrid(grid, 1), options);
        return;
    }
    cols = ediGetGridColumns(grid);
    ncols = mprGetListLength(cols);
    rowOptions = mprCreateHash(0, MPR_HASH_STABLE);

    httpSetOption(rowOptions, EDATA("click"), httpGetOption(options, EDATA("click"), 0));
    httpRemoveOption(options, EDATA("click"));
    httpSetOption(rowOptions, EDATA("remote"), httpGetOption(options, EDATA("remote"), 0));
    httpSetOption(rowOptions, EDATA("key"), httpGetOption(options, EDATA("key"), 0));
    httpSetOption(rowOptions, EDATA("params"), httpGetOption(options, EDATA("params"), 0));
    httpSetOption(rowOptions, EDATA("edit"), httpGetOption(options, EDATA("edit"), 0));

    httpInsertOption(options, "class", ESTYLE("table"));
    httpInsertOption(options, "class", ESTYLE("stripe"));
    espRender(conn, "<table%s>\r\n", map(conn, options));

    /*
        Table header
     */
    if (httpOption(options, "showHeader", "true", 1)) {
        espRender(conn, "    <thead>\r\n");
        if ((title = httpGetOption(options, "title", 0)) != 0) {
            espRender(conn, "        <tr class='" ESTYLE("table-title") "'><td colspan='%s'>%s</td></tr>\r\n", 
                mprGetListLength(cols), title);
        }
        espRender(conn, "        <tr class='" ESTYLE("table-header") "'>\r\n");
        rec = grid->records[0];
        for (c = 0; c < ncols; c++) {
            assert(c <= rec->nfields);
            fp = &rec->fields[c];
            width = ((o = httpGetOption(options, "width", 0)) != 0) ? sfmt(" width='%s'", o) : "";
            thisCol = mprLookupKey(colOptions, itosbuf(index, sizeof(index), c, 10));
            header = httpGetOption(thisCol, "header", stitle(fp->name));
            espRender(conn, "            <th%s>%s</th>\r\n", width, header);
        }
        espRender(conn, "        </tr>\r\n    </thead>\r\n");
    }
    espRender(conn, "    <tbody>\r\n");

    /*
        Table body data
     */
    for (r = 0; r < grid->nrecords; r++) {
        rec = grid->records[r];
        httpSetOption(rowOptions, "id", rec->id);
        espRender(conn, "        <tr%s>\r\n", map(conn, rowOptions));
        for (c = 0; c < ncols; c++) {
            fp = &rec->fields[c];
            thisCol = mprLookupKey(colOptions, itosbuf(index, sizeof(index), c, 10));

            if (httpGetOption(thisCol, "align", 0) == 0) {
                if (fp->type == EDI_TYPE_INT || fp->type == EDI_TYPE_FLOAT) {
                    if (!thisCol) {
                        thisCol = mprCreateHash(0, MPR_HASH_STABLE);
                    }
                    httpInsertOption(thisCol, "align", "right");
                }
            }
            value = formatValue(fp, thisCol);
            espRender(conn, "            <td%s>%s</td>\r\n", map(conn, thisCol), value);
        }
        espRender(conn, "        </tr>\r\n");
    }
    espRender(conn, "    </tbody>\r\n</table>\r\n");
}


/*
    tabs(makeGrid("[{ name: 'Status', uri: 'pane-1' }, { name: 'Edit', uri: 'pane-2' }]"))

    Options:
        click
        toggle
        remote
 */
PUBLIC void espTabs(HttpConn *conn, EdiGrid *grid, cchar *optionString)
{
    MprHash     *options;
    EdiRec      *rec;
    cchar       *attr, *uri, *name, *klass;
    int         r, toggle;

    options = httpGetOptions(optionString);
    httpInsertOption(options, "class", ESTYLE("tabs"));

    attr = httpGetOption(options, "toggle", EDATA("click"));
    if ((toggle = smatch(attr, "true")) != 0) {
        attr = EDATA("toggle");
    }
    espRender(conn, "<div%s>\r\n    <ul>\r\n", map(conn, options));
    for (r = 0; r < grid->nrecords; r++) {
        rec = grid->records[r];
        name = ediGetFieldValue(rec, "name");
        uri = ediGetFieldValue(rec, "uri");
        uri = toggle ? uri : httpLink(conn, uri);
        if ((r == 0 && toggle) || smatch(uri, conn->rx->pathInfo)) {
            klass = smatch(uri, conn->rx->pathInfo) ? " class='esp-selected'" : "";
        } else {
            klass = "";
        }
        espRender(conn, "          <li %s='%s'%s>%s</li>\r\n", attr, uri, klass, name);
    }
    espRender(conn, "        </ul>\r\n    </div>\r\n");
}


static void textInner(HttpConn *conn, cchar *field, MprHash *options)
{
    cchar   *rows, *cols, *type, *value;

    type = "text";
    value = getValue(conn, field, options);
    if (value == 0 || *value == '\0') {
        value = espGetParam(conn, field, "");
    }
    if (httpGetOption(options, "password", 0)) {
        type = "password";
    } else if (httpGetOption(options, "hidden", 0)) {
        type = "hidden";
    }
    if ((rows = httpGetOption(options, "rows", 0)) != 0) {
        cols = httpGetOption(options, "cols", "60");
        espRender(conn, "<textarea name='%s' type='%s' cols='%s' rows='%s'%s>%s</textarea>", field, type, 
            cols, rows, map(conn, options), value);
    } else {
          espRender(conn, "<input name='%s' type='%s' value='%s'%s />", field, type, value, map(conn, options));
    }
}


PUBLIC void espText(HttpConn *conn, cchar *field, cchar *optionString)
{
    textInner(conn, field, httpGetOptions(optionString));
}


/************************************ Abbreviated API *********************************/ 

PUBLIC void alert(cchar *text, cchar *optionString)
{
    espAlert(getConn(), text, optionString);
}


PUBLIC void anchor(cchar *text, cchar *uri, cchar *optionString) 
{
    espAnchor(getConn(), text, uri, optionString);
}


PUBLIC void button(cchar *name, cchar *value, cchar *optionString)
{
    espButton(getConn(), name, value, optionString);
}


PUBLIC void buttonLink(cchar *text, cchar *uri, cchar *optionString)
{
    espButtonLink(getConn(), text, uri, optionString);
}


PUBLIC void checkbox(cchar *field, cchar *checkedValue, cchar *optionString) 
{
    espCheckbox(getConn(), field, checkedValue, optionString);
}


PUBLIC void division(cchar *body, cchar *optionString) 
{
    espDivision(getConn(), body, optionString);
}

PUBLIC void endform() 
{
    renderString("</form>");
}


static void emitFormErrors(HttpConn *conn, EdiRec *rec, MprHash *options)
{
    MprHash         *errors;
    MprKey          *field;
    char            *msg;
    int             count;
   
    if (!rec->errors || httpGetOption(options, "hideErrors", 0)) {
        return;
    }
    errors = ediGetRecErrors(rec);
    if (errors) {
        count = mprGetHashLength(errors);
        espRender(conn, "<div class='" ESTYLE("form-error") "'><h2>The %s has %s it being saved.</h2>\r\n",
            stitle(rec->tableName), count <= 1 ? "an error that prevents" : "errors that prevent");
        espRender(conn, "    <p>There were problems with the following fields:</p>\r\n");
        espRender(conn, "    <ul>\r\n");
        for (ITERATE_KEY_DATA(errors, field, msg)) {
            espRender(conn, "        <li>%s %s</li>\r\n", field->key, msg);
        }
        espRender(conn, "    </ul>\r\n");
        espRender(conn, "</div>");
    }
}


PUBLIC void form(EdiRec *record, cchar *optionString)
{
    HttpConn    *conn;
    MprHash     *options;
    cchar       *action, *recid, *method, *uri, *token;
   
    conn = getConn();
    if (record == 0) {
        record = getRec();
    } else {
        conn->record = record;
    }
    options = httpGetOptions(optionString);
    recid = 0;

    /*
        If record provided, get the record id. Can be overridden using options.recid
     */
    if (record) {
        if (record->id && !httpGetOption(options, "recid", 0)) {
            httpAddOption(options, "recid", record->id);
        }
        recid = httpGetOption(options, "recid", 0);
        emitFormErrors(conn, record, options);
    }
    if ((method = httpGetOption(options, "method", 0)) == 0) {
        method = (recid) ? "PUT" : "POST";
    }
    if (!scaselessmatch(method, "GET") && !scaselessmatch(method, "POST")) {
        /* All methods use POST and tunnel method in data-method */
        httpAddOption(options, EDATA("method"), method);
        method = "POST";
    }
    if ((action = httpGetOption(options, "action", 0)) == 0) {
        action = (recid) ? "@update" : "@create";
    }
    uri = httpLink(conn, action);

    if (smatch(httpGetOption(options, "remote", 0), "true")) {
        espRender(conn, "<form method='%s' " EDATA("remote") "='%s'%s >\r\n", method, uri, map(conn, options));
    } else {
        espRender(conn, "<form method='%s' action='%s'%s >\r\n", method, uri, map(conn, options));
    }
    if (recid) {
        espRender(conn, "    <input name='recid' type='hidden' value='%s' />\r\n", recid);
    }
    if (!httpGetOption(options, "insecure", 0)) {
        if ((token = httpGetOption(options, "securityToken", 0)) == 0) {
            token = httpGetSecurityToken(conn, 0);
        }
        espRender(conn, "    <input name='%s' type='hidden' value='%s' />\r\n", ME_XSRF_PARAM, token);
    }
}


PUBLIC void icon(cchar *uri, cchar *optionString)
{
    espIcon(getConn(), uri, optionString);
}


PUBLIC void image(cchar *src, cchar *optionString)
{
    espImage(getConn(), src, optionString);
}


PUBLIC void inform(cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    espSetFlashv(getConn(), "inform", fmt, args);
    va_end(args);
}


PUBLIC void label(cchar *text, cchar *optionString)
{
    espLabel(getConn(), text, optionString);
}


PUBLIC void dropdown(cchar *name, EdiGrid *choices, cchar *optionString) 
{
    espDropdown(getConn(), name, choices, optionString);
}


PUBLIC void mail(cchar *name, cchar *address, cchar *optionString) 
{
    espMail(getConn(), name, address, optionString);
}


PUBLIC void progress(cchar *data, cchar *optionString)
{
    espProgress(getConn(), data, optionString);
}


/*
    radio("priority", "{low: 0, med: 1, high: 2}", NULL)
    radio("priority", "{low: 0, med: 1, high: 2}", "{value:'2'}")
 */
PUBLIC void radio(cchar *name, void *choices, cchar *optionString)
{
    espRadio(getConn(), name, choices, optionString);
}


PUBLIC void refresh(cchar *on, cchar *off, cchar *optionString)
{
    espRefresh(getConn(), on, off, optionString);
}


PUBLIC void script(cchar *uri, cchar *optionString)
{
    espScript(getConn(), uri, optionString);
}


PUBLIC void stylesheet(cchar *uri, cchar *optionString) 
{
    espStylesheet(getConn(), uri, optionString);
}


PUBLIC void table(EdiGrid *grid, cchar *optionString)
{
    if (grid == 0) {
        grid = getGrid();
    }
    espTable(getConn(), grid, optionString);
}


PUBLIC void tabs(EdiGrid *grid, cchar *optionString)
{
    espTabs(getConn(), grid, optionString);
}


PUBLIC void text(cchar *field, cchar *optionString)
{
    espText(getConn(), field, optionString);
}

/**************************************** Support *************************************/ 

static cchar *escapeValue(cchar *value, MprHash *options)
{
    if (httpGetOption(options, "escape", 0)) {
        return mprEscapeHtml(value);
    }
    return value;
}


static cchar *formatValue(EdiField *fp, MprHash *options)
{
    return ediFormatField(httpGetOption(options, "format", 0), fp);
}


static cchar *getValue(HttpConn *conn, cchar *fieldName, MprHash *options)
{
    EdiRec      *record;
    MprKey      *field;
    cchar       *value, *msg;

    record = conn->record;
    value = 0;

    if (record) {
        value = ediGetFieldValue(record, fieldName);
        if (record->errors) {
            for (ITERATE_KEY_DATA(record->errors, field, msg)) {
                if (smatch(field->key, fieldName)) {
                    httpInsertOption(options, "class", ESTYLE("field-error"));
                }
            }
        }
    }
    if (value == 0) {
        value = httpGetOption(options, "value", 0);
    }
    if (httpGetOption(options, "escape", 0)) {
        value = mprEscapeHtml(value);
    }
    return value;
}


/*
    Map options to an attribute string. Remove all internal control specific options and transparently handle URI link options.
    WARNING: this returns a non-cloned reference and relies on no GC yield until the returned value is used or cloned.
    This is done as an optimization to reduce memeory allocations.
 */
static cchar *map(HttpConn *conn, MprHash *options)
{
    Esp         *esp;
    EspReq      *req;
    MprHash     *params;
    MprKey      *kp;
    MprBuf      *buf;
    cchar       *value;
    char        *pstr;

    if (options == 0 || mprGetHashLength(options) == 0) {
        return MPR->emptyString;
    }
    req = conn->data;
    if (httpGetOption(options, EDATA("refresh"), 0) && !httpGetOption(options, "id", 0)) {
        httpAddOption(options, "id", sfmt("id_%d", req->lastDomID++));
    }
    esp = MPR->espService;
    buf = mprCreateBuf(-1, -1);
    for (kp = 0; (kp = mprGetNextKey(options, kp)) != 0; ) {
        if (kp->type != MPR_JSON_OBJ && kp->type != MPR_JSON_ARRAY && !mprLookupKey(esp->internalOptions, kp->key)) {
            mprPutCharToBuf(buf, ' ');
            value = kp->data;
            /*
                Support link template resolution for these options
             */
            if (smatch(kp->key, EDATA("click")) || smatch(kp->key, EDATA("remote")) || smatch(kp->key, EDATA("refresh"))) {
                value = httpLinkEx(conn, value, options);
                if ((params = httpGetOptionHash(options, "params")) != 0) {
                    pstr = (char*) "";
                    for (kp = 0; (kp = mprGetNextKey(params, kp)) != 0; ) {
                        pstr = sjoin(pstr, mprUriEncode(kp->key, MPR_ENCODE_URI_COMPONENT), "=", 
                            mprUriEncode(kp->data, MPR_ENCODE_URI_COMPONENT), "&", NULL);
                    }
                    if (pstr[0]) {
                        /* Trim last "&" */
                        pstr[strlen(pstr) - 1] = '\0';
                    }
                    mprPutToBuf(buf, "params='%s'", pstr);
                }
            }
            mprPutStringToBuf(buf, kp->key);
            mprPutStringToBuf(buf, "='");
            mprPutStringToBuf(buf, value);
            mprPutCharToBuf(buf, '\'');
        }
    }
    mprAddNullToBuf(buf);
    return mprGetBufStart(buf);
}


PUBLIC void espInitHtmlOptions(Esp *esp)
{
    char   **op;

    esp->internalOptions = mprCreateHash(-1, MPR_HASH_STATIC_VALUES);
    for (op = internalOptions; *op; op++) {
        mprAddKey(esp->internalOptions, *op, op);
    }
}

#endif /* ME_COM_ESP && ME_ESP_LEGACY */
/*
    @copy   default

    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
/************************************************************************/
/*
    Start of file "src/espFramework.c"
 */
/************************************************************************/
/*
    espFramework.c -- ESP Web Framework API

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#if ME_COM_ESP
/************************************ Defines *********************************/

#define ITERATE_CONFIG(obj, child, index) \
    index = 0, child = obj ? obj->children: 0; obj && index < obj->length; child = child->next, index++

/************************************* Code ***********************************/

PUBLIC void espAddPak(HttpRoute *route, cchar *name, cchar *version)
{
    EspRoute    *eroute;

    eroute = route->eroute;
    if (!version || !*version || smatch(version, "0.0.0")) {
        version = "*";
    }
    mprSetJson(eroute->config, sfmt("dependencies.%s", name), version);
}


/* 
    Add a http header if not already defined
 */
PUBLIC void espAddHeader(HttpConn *conn, cchar *key, cchar *fmt, ...)
{
    va_list     vargs;

    assert(key && *key);
    assert(fmt && *fmt);

    va_start(vargs, fmt);
    httpAddHeaderString(conn, key, sfmt(fmt, vargs));
    va_end(vargs);
}


/*
    Add a header string if not already defined
 */
PUBLIC void espAddHeaderString(HttpConn *conn, cchar *key, cchar *value)
{
    httpAddHeaderString(conn, key, value);
}


PUBLIC void espAddParam(HttpConn *conn, cchar *var, cchar *value) 
{
    if (!httpGetParam(conn, var, 0)) {
        httpSetParam(conn, var, value);
    }
}


/* 
   Append a header. If already defined, the value is catenated to the pre-existing value after a ", " separator.
   As per the HTTP/1.1 spec.
 */
PUBLIC void espAppendHeader(HttpConn *conn, cchar *key, cchar *fmt, ...)
{
    va_list     vargs;

    assert(key && *key);
    assert(fmt && *fmt);

    va_start(vargs, fmt);
    httpAppendHeaderString(conn, key, sfmt(fmt, vargs));
    va_end(vargs);
}


/* 
   Append a header string. If already defined, the value is catenated to the pre-existing value after a ", " separator.
   As per the HTTP/1.1 spec.
 */
PUBLIC void espAppendHeaderString(HttpConn *conn, cchar *key, cchar *value)
{
    httpAppendHeaderString(conn, key, value);
}


PUBLIC void espAutoFinalize(HttpConn *conn) 
{
    EspReq  *req;

    req = conn->data;
    if (req->autoFinalize) {
        httpFinalize(conn);
    }
}


PUBLIC int espCache(HttpRoute *route, cchar *uri, int lifesecs, int flags)
{
    httpAddCache(route, NULL, uri, NULL, NULL, 0, lifesecs * MPR_TICKS_PER_SEC, flags);
    return 0;
}


#if ME_ESP_LEGACY
/*
    Should not be called explicitly
 */
PUBLIC bool espCheckSecurityToken(HttpConn *conn) 
{
    return httpCheckSecurityToken(conn);
}
#endif


PUBLIC cchar *espCreateSession(HttpConn *conn)
{
    HttpSession *session;

    if ((session = httpCreateSession(getConn())) != 0) {
        return session->id;
    }
    return 0;
}


PUBLIC void espDefineAction(HttpRoute *route, cchar *target, void *action)
{
    EspRoute    *eroute;
    Esp         *esp;

    assert(route);
    assert(target && *target);
    assert(action);

    esp = MPR->espService;
    if (target) {
        eroute = route->eroute;
        mprAddKey(esp->actions, mprJoinPath(eroute->controllersDir, target), action);
    }
}


/*
    The base procedure is invoked prior to calling any and all actions on this route
 */
PUBLIC void espDefineBase(HttpRoute *route, EspProc baseProc)
{
    HttpRoute   *rp;
    EspRoute    *eroute, *er;
    int         next;

    eroute = route->eroute;
    for (ITERATE_ITEMS(route->host->routes, rp, next)) {
        if ((er = route->eroute) != 0 && smatch(er->controllersDir, eroute->controllersDir)) {
            er->commonController = baseProc;
        }
    }
}


/*
    Path should be an app-relative path to the view file (relative-path.esp)
 */
PUBLIC void espDefineView(HttpRoute *route, cchar *path, void *view)
{
    Esp         *esp;

    assert(path && *path);
    assert(view);

    if (!route) {
        mprError("Route not defined for view %s", path);
        return;
    }
    esp = MPR->espService;
    path = mprGetPortablePath(mprJoinPath(route->documents, path));
    mprAddKey(esp->views, path, view);
}


PUBLIC void espDestroySession(HttpConn *conn)
{
    httpDestroySession(conn);
}


PUBLIC void espFinalize(HttpConn *conn) 
{
    httpFinalize(conn);
}


PUBLIC void espFlush(HttpConn *conn) 
{
    httpFlush(conn);
}


PUBLIC cchar *espGetConfig(HttpRoute *route, cchar *key, cchar *defaultValue)
{
    EspRoute    *eroute;
    cchar       *value;

    eroute = route->eroute;
    if ((value = mprGetJson(eroute->config, key)) != 0) {
        return value;
    }
    return defaultValue;
}


PUBLIC MprOff espGetContentLength(HttpConn *conn)
{
    return httpGetContentLength(conn);
}


PUBLIC cchar *espGetContentType(HttpConn *conn)
{
    return conn->rx->mimeType;
}


PUBLIC cchar *espGetCookie(HttpConn *conn, cchar *name)
{
    return httpGetCookie(conn, name);
}


PUBLIC cchar *espGetCookies(HttpConn *conn) 
{
    return httpGetCookies(conn);
}


PUBLIC void *espGetData(HttpConn *conn)
{
    EspReq  *req;

    req = conn->data;
    return req->data;
}


PUBLIC Edi *espGetDatabase(HttpConn *conn)
{
    HttpRx      *rx;
    EspReq      *req;
    EspRoute    *eroute;
    Edi         *edi;

    rx = conn->rx;
    req = conn->data;
    edi = req ? req->edi : 0;
    if (edi == 0 && rx && rx->route) {
        if ((eroute = rx->route->eroute) != 0) {
            edi = eroute->edi;
        }
    }
    if (edi == 0) {
        httpError(conn, 0, "Cannot get database instance");
        return 0;
    }
    return edi;
}


PUBLIC cchar *espGetDocuments(HttpConn *conn)
{   
    return conn->rx->route->documents;
}


#if ME_ESP_LEGACY
PUBLIC cchar *espGetDir(HttpConn *conn)
{
    return espGetDocuments(conn);
}
#endif


PUBLIC EspRoute *espGetEspRoute(HttpConn *conn)
{
    return conn->rx->route->eroute;
}


PUBLIC cchar *espGetFlash(HttpConn *conn, cchar *kind)
{
    EspReq      *req;
    MprKey      *kp;
    cchar       *msg;
   
    req = conn->data;
    if (kind == 0 || req->flash == 0 || mprGetHashLength(req->flash) == 0) {
        return 0;
    }
    for (kp = 0; (kp = mprGetNextKey(req->flash, kp)) != 0; ) {
        msg = kp->data;
        if (smatch(kind, kp->key) || smatch(kind, "all")) {
            return msg;
        }
    }
    return 0;
}


PUBLIC cchar *espGetFeedback(HttpConn *conn, cchar *kind)
{
    EspReq      *req;
    MprKey      *kp;
    cchar       *msg;
   
    req = conn->data;
    if (kind == 0 || req == 0 || req->feedback == 0 || mprGetHashLength(req->feedback) == 0) {
        return 0;
    }
    for (kp = 0; (kp = mprGetNextKey(req->feedback, kp)) != 0; ) {
        msg = kp->data;
        if (smatch(kind, kp->key) || smatch(kind, "all")) {
            return msg;
        }
    }
    return 0;

}


PUBLIC EdiGrid *espGetGrid(HttpConn *conn)
{           
    return conn->grid;
}


PUBLIC cchar *espGetHeader(HttpConn *conn, cchar *key)
{
    return httpGetHeader(conn, key);
}


PUBLIC MprHash *espGetHeaderHash(HttpConn *conn)
{
    return httpGetHeaderHash(conn);
}


PUBLIC char *espGetHeaders(HttpConn *conn)
{
    return httpGetHeaders(conn);
}


PUBLIC int espGetIntParam(HttpConn *conn, cchar *var, int defaultValue)
{
    return httpGetIntParam(conn, var, defaultValue);
}

  
PUBLIC cchar *espGetMethod(HttpConn *conn)
{   
    return conn->rx->method;
} 


PUBLIC cchar *espGetParam(HttpConn *conn, cchar *var, cchar *defaultValue)
{
    return httpGetParam(conn, var, defaultValue);
}


PUBLIC MprJson *espGetParams(HttpConn *conn)
{
    return httpGetParams(conn);
}


PUBLIC cchar *espGetQueryString(HttpConn *conn)
{
    return httpGetQueryString(conn);
}


PUBLIC char *espGetReferrer(HttpConn *conn)
{
    if (conn->rx->referrer) {
        return conn->rx->referrer;
    }
    return httpLink(conn, "~");
}


PUBLIC Edi *espGetRouteDatabase(HttpRoute *route)
{
    EspRoute    *eroute;

    eroute = route->eroute;
    if (eroute == 0 || eroute->edi == 0) {
        return 0;
    }
    return eroute->edi;
}


PUBLIC cchar *espGetSessionID(HttpConn *conn, int create)
{
    HttpSession *session;

    if ((session = httpGetSession(getConn(), create)) != 0) {
        return session->id;
    }
    return 0;
}


PUBLIC int espGetStatus(HttpConn *conn)
{
    return httpGetStatus(conn);
}


PUBLIC char *espGetStatusMessage(HttpConn *conn)
{
    return httpGetStatusMessage(conn);
}


#if ME_ESP_LEGACY
PUBLIC char *espGetTop(HttpConn *conn)
{
    return httpLink(conn, "~");
}
#endif


PUBLIC MprList *espGetUploads(HttpConn *conn)
{
    return conn->rx->files;
}


PUBLIC cchar *espGetUri(HttpConn *conn)
{
    return conn->rx->uri;
}


PUBLIC bool espHasPak(HttpRoute *route, cchar *name)
{
    EspRoute    *eroute;

    eroute = route->eroute;
    return mprGetJsonObj(eroute->config, sfmt("dependencies.%s", name)) != 0;
}


PUBLIC bool espHasGrid(HttpConn *conn)
{
    return conn->grid != 0;
}


PUBLIC bool espHasRec(HttpConn *conn)
{
    EdiRec  *rec;

    rec = conn->record;
    return (rec && rec->id) ? 1 : 0;
}


PUBLIC bool espIsEof(HttpConn *conn)
{
    return httpIsEof(conn);
}


PUBLIC bool espIsFinalized(HttpConn *conn) 
{
    return httpIsFinalized(conn);
}


PUBLIC bool espIsSecure(HttpConn *conn)
{
    return conn->secure;
}


static void clientCopy(EspRoute *eroute, MprJson *dest, MprJson *obj)
{
    MprJson     *child, *job, *value;
    int         ji;

    for (ITERATE_CONFIG(obj, child, ji)) {
        if (child->type & MPR_JSON_OBJ) {
            job = mprCreateJson(MPR_JSON_OBJ);
            clientCopy(eroute, job, child);
            mprSetJsonObj(dest, child->name, job);
        } else {
            if ((value = mprGetJsonObj(eroute->config, child->value)) != 0) {
                mprSetJsonObj(dest, child->name, mprCloneJson(value));
            }
        }
    }
}


static void createRedirectAlias(HttpRoute *route, int status, cchar *from, cchar *to)
{
    HttpRoute   *alias;
    cchar       *pattern, *prefix;

    if (from == 0 || *from == '\0') {
        from = "/";
    }
    prefix = route->prefix ? route->prefix : "";

    if (sends(from, "/")) {
        pattern = sfmt("^%s%s(.*)$", prefix, from);
    } else {
        /* Add a non-capturing optional trailing "/" */
        pattern = sfmt("^%s%s(?:/)*(.*)$", prefix, from);
    }
    alias = httpCreateAliasRoute(route, pattern, 0, 0);
    httpSetRouteMethods(alias, "*");
    httpSetRouteTarget(alias, "redirect", sfmt("%d %s/$1", status, to));
    if (sstarts(to, "https")) {
        httpAddRouteCondition(alias, "secure", 0, HTTP_ROUTE_NOT);
    }
    httpFinalizeRoute(alias);
}


static cchar *getList(MprJson *prop)
{
    char    *jstr, *cp;

    if (prop == 0) {
        return 0;
    }
    if ((jstr = mprJsonToString(prop, 0)) == 0) {
        return 0;
    }
    if (*jstr == '[') {
        jstr = strim(jstr, "[]", 0);
    }
    for (cp = jstr; *cp; cp++) {
        if (*cp == '"') {
            *cp = ' ';
        }
    }
    return jstr;
}


static void parseCache(HttpRoute *route, MprJson *prop)
{
    MprJson     *child;
    MprTicks    clientLifespan, serverLifespan;
    cchar       *methods, *extensions, *uris, *mimeTypes, *client, *server;
    int         flags, ji;

    serverLifespan = clientLifespan = 0;
    for (ITERATE_CONFIG(prop, child, ji)) {
        flags = 0;
        if ((client = mprGetJson(child, "client")) != 0) {
            flags |= HTTP_CACHE_CLIENT;
            clientLifespan = httpGetTicks(client);
        }
        if ((server = mprGetJson(child, "server")) != 0) {
            flags |= HTTP_CACHE_SERVER;
            serverLifespan = httpGetTicks(server);
        }
        methods = getList(mprGetJsonObj(child, "methods"));
        extensions = getList(mprGetJsonObj(child, "extensions"));
        uris = getList(mprGetJsonObj(child, "uris"));
        mimeTypes = getList(mprGetJsonObj(child, "mime"));

        if (smatch(mprGetJson(child, "unique"), "true")) {
            /* Uniquely cache requests with different params */
            flags |= HTTP_CACHE_UNIQUE;
        }
        if (smatch(mprGetJson(child, "manual"), "true")) {
            /* User must manually call httpWriteCache */
            flags |= HTTP_CACHE_MANUAL;
        }
        httpAddCache(route, methods, uris, extensions, mimeTypes, clientLifespan, serverLifespan, flags);
    }
}


/*
    Load the package.json
 */
PUBLIC int espLoadConfig(HttpRoute *route)
{
    EspRoute    *eroute;
    MprJson     *msettings, *settings, *client, *mappings, *prop, *child;
    MprPath     cinfo;
    cchar       *cdata, *cpath, *value, *errorMsg, *from, *status, *to;
    bool        debug;
    int         ji;

    eroute = route->eroute;
    lock(eroute);

    /*
        See if config file has been modified and if so, reload.
     */
    cpath = mprJoinPath(route->documents, ME_ESP_PACKAGE);
    if (mprGetPathInfo(cpath, &cinfo) == 0) {
        if (eroute->config && cinfo.mtime > eroute->configLoaded) {
            eroute->config = 0;
        }
        eroute->configLoaded = cinfo.mtime;
    }
    if (!eroute->config) {
#if ME_ESP_LEGACY
        if (!mprPathExists(cpath, R_OK)) {
            if (!eroute->legacy) {
                mprLog(2, "esp: Missing %s, switching to esp-legacy-mvc mode", cpath);
            }
            eroute->legacy = 1;
        } else 
#endif
        {
            if ((cdata = mprReadPathContents(cpath, NULL)) == 0) {
                mprError("Cannot read ESP configuration from %s", cpath);
                unlock(eroute);
                return MPR_ERR_CANT_READ;
            }
            if ((eroute->config = mprParseJsonEx(cdata, 0, 0, 0, &errorMsg)) == 0) {
                mprError("Cannot parse %s: error %s", cpath, errorMsg);
                unlock(eroute);
                return 0;
            }
            /*
                Blend the mode properties into settings
             */
            eroute->mode = mprGetJson(eroute->config, "app.mode");
            if (!eroute->mode) {
                eroute->mode = sclone("debug");
                mprLog(2, "esp: application \"%s\" running in \"%s\" mode", eroute->appName, eroute->mode);
            }
            debug = smatch(eroute->mode, "debug");
            if ((msettings = mprGetJsonObj(eroute->config, sfmt("app.modes.%s", eroute->mode))) != 0) {
                settings = mprLookupJsonObj(eroute->config, "app");
                mprBlendJson(settings, msettings, MPR_JSON_OVERWRITE);
                mprSetJson(settings, "app.mode", eroute->mode);
            }
            if ((value = espGetConfig(route, "app.http.auth.store", 0)) != 0) {
                if (httpSetAuthStore(route->auth, value) < 0) {
                    mprError("The %s AuthStore is not available on this platform", value);
                }
            }
            if (espGetConfig(route, "app.http.content.combine[@=c]", 0)) {
                eroute->combined = 1;
                mprLog(2, "esp: app %s configured for combined compilation", eroute->appName);
            }
            if ((value = espGetConfig(route, "app.esp.compile", 0)) != 0) {
                if (smatch(value, "debug") || smatch(value, "symbols")) {
                    eroute->compileMode = ESP_COMPILE_DEBUG;
                } else if (smatch(value, "release") || smatch(value, "optimized")) {
                    eroute->compileMode = ESP_COMPILE_RELEASE;
                }
            }
            if ((prop = mprGetJsonObj(eroute->config, "app.http.redirect")) != 0) {
                if (prop->type & MPR_JSON_STRING) {
                    createRedirectAlias(route, 0, "/", prop->value);
                } else {
                    for (ITERATE_CONFIG(prop, child, ji)) {
                        if (child->type & MPR_JSON_STRING) {
                            from = "/";
                            to = child->value;
                            status = "302";
                        } else {
                            from = mprGetJson(child, "from");
                            to = mprGetJson(child, "to");
                            status = mprGetJson(child, "status");
                        }
                        createRedirectAlias(route, (int) stoi(status), from, to);
                    }
                }
            }
            if ((value = espGetConfig(route, "app.http.showErrors", 0)) != 0) {
                httpSetRouteShowErrors(route, smatch(value, "true"));
            } else if (debug) {
                httpSetRouteShowErrors(route, 1);
            }
            if ((value = espGetConfig(route, "app.http.update", 0)) != 0) {
                eroute->update = smatch(value, "true");
            }
            if ((value = espGetConfig(route, "app.http.keep", 0)) != 0) {
                eroute->keepSource = smatch(value, "true");
            }
            if ((value = espGetConfig(route, "app.http.serverPrefix", 0)) != 0) {
                httpSetRouteServerPrefix(route, value);
                httpSetRouteVar(route, "SERVER_PREFIX", sjoin(route->prefix ? route->prefix: "", route->serverPrefix, 0));
            }
            if ((value = espGetConfig(route, "app.http.auth.login.name", 0)) != 0) {
                /* Automatic login as this user. Password not required */
                httpSetAuthUsername(route->auth, value);
            }
            if ((value = espGetConfig(route, "app.http.xsrf", 0)) != 0) {
                httpSetRouteXsrf(route, smatch(value, "true"));
            } else {
                httpSetRouteXsrf(route, 1);
            }
            if ((value = espGetConfig(route, "app.http.formats.response", 0)) != 0) {
                if (smatch(value, "json")) {
                    eroute->json = smatch(value, "true");
                }
            }
            if ((value = espGetConfig(route, "app.http.timeouts.session", 0)) != 0) {
                route->limits->sessionTimeout = httpGetTicks(value);
                mprLog(2, "esp: set session timeout to %s", value);
            }
            if ((prop = mprGetJsonObj(eroute->config, "app.http.content.compress")) != 0) {
                for (ITERATE_CONFIG(prop, child, ji)) {
                    if (mprGetJson(eroute->config, sfmt("app.http.content.minify[@ = '%s']", child->value))) {
                        httpAddRouteMapping(route, child->value, "${1}.gz, min.${1}.gz, min.${1}");
                    } else {
                        httpAddRouteMapping(route, child->value, "${1}.gz");
                    }
                }
            }
            if ((prop = mprGetJsonObj(eroute->config, "app.http.cache")) != 0) {
                parseCache(route, prop);
            }
            if (!eroute->database) {
                if ((eroute->database = espGetConfig(route, "app.http.database", 0)) != 0) {
                    if (espOpenDatabase(route, eroute->database) < 0) {
                        mprError("Cannot open database %s", eroute->database);
                        unlock(eroute);
                        return MPR_ERR_CANT_OPEN;
                    }
                }
            }
#if ME_ESP_LEGACY
            if (espHasPak(route, "esp-legacy-mvc")) {
                eroute->legacy = 1;
            }
#endif
        }
#if ME_ESP_LEGACY
        if (eroute->legacy) {
            espSetLegacyDirs(route);
            httpSetRouteServerPrefix(route, "");
            if (!eroute->config) {
                eroute->config = mprCreateJson(MPR_JSON_OBJ);
                espAddPak(route, "esp-legacy-mvc", 0);
            }
        }
#endif
    }
    /*
        Create a subset, optimized configuration to send to the client
     */
    if ((mappings = mprGetJsonObj(eroute->config, "app.client.mappings")) != 0) {
        client = mprCreateJson(MPR_JSON_OBJ);
        clientCopy(eroute, client, mappings);
        mprSetJson(client, "prefix", route->prefix ? route->prefix : "");
        eroute->client = mprJsonToString(client, MPR_JSON_QUOTES);
        mprTrace(6, "Client Config: for %s, %s", route->name, mprJsonToString(client, MPR_JSON_PRETTY));
    }
    unlock(eroute);
    return 0;
}


PUBLIC bool espMatchParam(HttpConn *conn, cchar *var, cchar *value)
{
    return httpMatchParam(conn, var, value);
}


/*
    Read rx data in non-blocking mode. Use standard connection timeouts. 
 */
PUBLIC ssize espReceive(HttpConn *conn, char *buf, ssize len)
{
    return httpRead(conn, buf, len);
}


PUBLIC void espRedirect(HttpConn *conn, int status, cchar *target)
{
    httpRedirect(conn, status, target);
}


PUBLIC void espRedirectBack(HttpConn *conn)
{
    if (conn->rx->referrer) {
        espRedirect(conn, HTTP_CODE_MOVED_TEMPORARILY, conn->rx->referrer); 
    }
}


PUBLIC ssize espRender(HttpConn *conn, cchar *fmt, ...)
{
    va_list     vargs;
    char        *buf;

    va_start(vargs, fmt);
    buf = sfmtv(fmt, vargs);
    va_end(vargs);
    return espRenderString(conn, buf);
}
    

PUBLIC ssize espRenderBlock(HttpConn *conn, cchar *buf, ssize size)
{
    return httpWriteBlock(conn->writeq, buf, size, HTTP_BUFFER);
}


PUBLIC ssize espRenderCached(HttpConn *conn)
{
    return httpWriteCached(conn);
}


PUBLIC ssize espRenderError(HttpConn *conn, int status, cchar *fmt, ...)
{
    va_list     args;
    HttpRx      *rx;
    ssize       written;
    cchar       *msg, *title, *text;

    va_start(args, fmt);    

    rx = conn->rx;
    written = 0;

    if (!httpIsFinalized(conn)) {
        if (status == 0) {
            status = HTTP_CODE_INTERNAL_SERVER_ERROR;
        }
        title = sfmt("Request Error for \"%s\"", rx->pathInfo);
        msg = mprEscapeHtml(sfmtv(fmt, args));
        if (rx->route->flags & HTTP_ROUTE_SHOW_ERRORS) {
            text = sfmt(\
                "<!DOCTYPE html>\r\n<html>\r\n<head><title>%s</title></head>\r\n" \
                "<body>\r\n<h1>%s</h1>\r\n" \
                "    <pre>%s</pre>\r\n" \
                "    <p>To prevent errors being displayed in the browser, " \
                "       set <b>ShowErrors off</b> in the appweb.conf file.</p>\r\n" \
                "</body>\r\n</html>\r\n", title, title, msg);
            httpSetHeader(conn, "Content-Type", "text/html");
            written += espRenderString(conn, text);
            espFinalize(conn);
            mprTrace(4, "Request error (%d) for: \"%s\"", status, rx->pathInfo);
        }
    }
    va_end(args);    
    return written;
}


PUBLIC ssize espRenderFile(HttpConn *conn, cchar *path)
{
    MprFile     *from;
    ssize       count, written, nbytes;
    char        buf[ME_MAX_BUFFER];

    if ((from = mprOpenFile(path, O_RDONLY | O_BINARY, 0)) == 0) {
        return MPR_ERR_CANT_OPEN;
    }
    written = 0;
    while ((count = mprReadFile(from, buf, sizeof(buf))) > 0) {
        if ((nbytes = espRenderBlock(conn, buf, count)) < 0) {
            return nbytes;
        }
        written += nbytes;
    }
    mprCloseFile(from);
    return written;
}


PUBLIC void espRenderFlash(HttpConn *conn, cchar *kinds)
{
    EspReq      *req;
    MprKey      *kp;
    cchar       *msg;
   
    req = conn->data;
    if (kinds == 0 || req->flash == 0 || mprGetHashLength(req->flash) == 0) {
        return;
    }
    for (kp = 0; (kp = mprGetNextKey(req->flash, kp)) != 0; ) {
        msg = kp->data;
        if (strstr(kinds, kp->key) || strstr(kinds, "all")) {
#if ME_ESP_LEGACY
            EspRoute *eroute = conn->rx->route->eroute;
            if (eroute->legacy) {
                espRender(conn, "<div class='esp-flash esp-flash-%s'>%s</div>", kp->key, msg);
            } else 
#endif
            espRender(conn, "<span class='feedback-%s animate'>%s</span>", kp->key, msg);
        }
    }
}


PUBLIC void espRemoveCookie(HttpConn *conn, cchar *name)
{
    httpSetCookie(conn, name, "", "/", NULL, -1, 0);
}


PUBLIC void espSetConn(HttpConn *conn)
{
    mprSetThreadData(((Esp*) MPR->espService)->local, conn);
}


static void espNotifier(HttpConn *conn, int event, int arg)
{
    EspReq      *req;

    if ((req = conn->data) != 0) {
        espSetConn(conn);
        (req->notifier)(conn, event, arg);
    }
}


PUBLIC void espSetNotifier(HttpConn *conn, HttpNotifier notifier)
{
    EspReq      *req;

    if ((req = conn->data) != 0) {
        req->notifier = notifier;
        httpSetConnNotifier(conn, espNotifier);
    }
}


PUBLIC ssize espRenderSafe(HttpConn *conn, cchar *fmt, ...)
{
    va_list     args;
    cchar       *s;

    va_start(args, fmt);
    s = mprEscapeHtml(sfmtv(fmt, args));
    va_end(args);
    return espRenderBlock(conn, s, slen(s));
}


PUBLIC ssize espRenderSafeString(HttpConn *conn, cchar *s)
{
    s = mprEscapeHtml(s);
    return espRenderBlock(conn, s, slen(s));
}


PUBLIC ssize espRenderString(HttpConn *conn, cchar *s)
{
    return espRenderBlock(conn, s, slen(s));
}


/*
    Render a request variable. If a param by the given name is not found, consult the session.
 */
PUBLIC ssize espRenderVar(HttpConn *conn, cchar *name)
{
    cchar   *value;

    if ((value = espGetParam(conn, name, 0)) == 0) {
        value = httpGetSessionVar(conn, name, "");
    }
    return espRenderSafeString(conn, value);
}


PUBLIC int espRemoveHeader(HttpConn *conn, cchar *key)
{
    assert(key && *key);
    if (conn->tx == 0) {
        return MPR_ERR_CANT_ACCESS;
    }
    return mprRemoveKey(conn->tx->headers, key);
}


PUBLIC void espRemoveSessionVar(HttpConn *conn, cchar *var) 
{
    httpRemoveSessionVar(conn, var);
}


PUBLIC int espSaveConfig(HttpRoute *route)
{
    EspRoute    *eroute;
    cchar       *path;

    eroute = route->eroute;
    path = mprJoinPath(route->documents, ME_ESP_PACKAGE);
#if KEEP
    mprBackupLog(path, 3);
#endif
    return mprSaveJson(eroute->config, path, MPR_JSON_PRETTY | MPR_JSON_QUOTES);
}


PUBLIC ssize espSendGrid(HttpConn *conn, EdiGrid *grid, int flags)
{
    httpAddHeaderString(conn, "Content-Type", "application/json");
    if (grid) {
        return espRender(conn, "{\n  \"data\": %s, \"schema\": %s}\n", ediGridAsJson(grid, flags), ediGetGridSchemaAsJson(grid));
    }
    return espRender(conn, "{}");
}


PUBLIC ssize espSendRec(HttpConn *conn, EdiRec *rec, int flags)
{
    httpAddHeaderString(conn, "Content-Type", "application/json");
    if (rec) {
        return espRender(conn, "{\n  \"data\": %s, \"schema\": %s}\n", ediRecAsJson(rec, flags), ediGetRecSchemaAsJson(rec)); 
    }
    return espRender(conn, "{}");
}


PUBLIC void espSendResult(HttpConn *conn, bool success)
{
    EspReq      *req;
    EdiRec      *rec;

    req = conn->data;
    rec = getRec();
    if (rec && rec->errors) {
        espRender(conn, "{\"error\": %d, \"feedback\": %s, \"fieldErrors\": %s}", !success, 
            req->feedback ? mprSerialize(req->feedback, MPR_JSON_QUOTES) : "{}",
            mprSerialize(rec->errors, MPR_JSON_QUOTES));
    } else {
        espRender(conn, "{\"error\": %d, \"feedback\": %s}", !success, 
            req->feedback ? mprSerialize(req->feedback, MPR_JSON_QUOTES) : "{}");
    }
    espFinalize(conn);
}


PUBLIC bool espSetAutoFinalizing(HttpConn *conn, bool on) 
{
    EspReq  *req;
    bool    old;

    req = conn->data;
    old = req->autoFinalize;
    req->autoFinalize = on;
    return old;
}


PUBLIC int espSetConfig(HttpRoute *route, cchar *key, cchar *value)
{
    EspRoute    *eroute;

    eroute = route->eroute;
    return mprSetJson(eroute->config, key, value);
}


PUBLIC void espSetContentLength(HttpConn *conn, MprOff length)
{
    httpSetContentLength(conn, length);
}


PUBLIC void espSetCookie(HttpConn *conn, cchar *name, cchar *value, cchar *path, cchar *cookieDomain, MprTicks lifespan, 
        bool isSecure)
{
    httpSetCookie(conn, name, value, path, cookieDomain, lifespan, isSecure);
}


PUBLIC void espSetContentType(HttpConn *conn, cchar *mimeType)
{
    httpSetContentType(conn, mimeType);
}


PUBLIC void espSetData(HttpConn *conn, void *data)
{
    EspReq  *req;

    req = conn->data;
    req->data = data;
}


PUBLIC void espSetFeedback(HttpConn *conn, cchar *kind, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    espSetFeedbackv(conn, kind, fmt, args);
    va_end(args);
}


PUBLIC void espSetFeedbackv(HttpConn *conn, cchar *kind, cchar *fmt, va_list args)
{
    EspReq      *req;
    cchar       *prior, *msg;

    if ((req = conn->data) == 0) {
        return;
    }
    msg = sfmtv(fmt, args);

    if (req->feedback == 0) {
        req->feedback = mprCreateHash(0, MPR_HASH_STABLE);
    }
    if ((prior = mprLookupKey(req->feedback, kind)) != 0) {
        mprAddKey(req->feedback, kind, sjoin(prior, ", ", msg, NULL));
    } else {
        mprAddKey(req->feedback, kind, sclone(msg));
    }
}


PUBLIC void espSetFlash(HttpConn *conn, cchar *kind, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    espSetFlashv(conn, kind, fmt, args);
    va_end(args);
}


PUBLIC void espSetFlashv(HttpConn *conn, cchar *kind, cchar *fmt, va_list args)
{
    EspReq      *req;
    cchar       *msg;

    req = conn->data;
    msg = sfmtv(fmt, args);

    if (req->flash == 0) {
        req->flash = mprCreateHash(0, MPR_HASH_STABLE);
    }
    mprAddKey(req->flash, kind, sclone(msg));
    /*
        Create a session as early as possible so a Set-Cookie header can be omitted.
     */
    httpGetSession(conn, 1);
}


PUBLIC EdiGrid *espSetGrid(HttpConn *conn, EdiGrid *grid)
{
    return conn->grid = grid;
}


/*  
    Set a http header. Overwrite if present.
 */
PUBLIC void espSetHeader(HttpConn *conn, cchar *key, cchar *fmt, ...)
{
    va_list     vargs;

    assert(key && *key);
    assert(fmt && *fmt);

    va_start(vargs, fmt);
    httpSetHeaderString(conn, key, sfmtv(fmt, vargs));
    va_end(vargs);
}


PUBLIC void espSetHeaderString(HttpConn *conn, cchar *key, cchar *value)
{
    httpSetHeaderString(conn, key, value);
}


PUBLIC void espSetIntParam(HttpConn *conn, cchar *var, int value) 
{
    httpSetIntParam(conn, var, value);
}


PUBLIC void espSetParam(HttpConn *conn, cchar *var, cchar *value) 
{
    httpSetParam(conn, var, value);
}


PUBLIC EdiRec *espSetRec(HttpConn *conn, EdiRec *rec)
{
    return conn->record = rec;
}


PUBLIC int espSetSessionVar(HttpConn *conn, cchar *var, cchar *value) 
{
    return httpSetSessionVar(conn, var, value);
}


PUBLIC void espSetStatus(HttpConn *conn, int status)
{
    httpSetStatus(conn, status);
}


PUBLIC void espShowRequest(HttpConn *conn)
{
    MprHash     *env;
    MprJson     *params, *param;
    MprKey      *kp;
    MprJson     *jkey;
    HttpRx      *rx;
    int         i;

    rx = conn->rx;
    httpAddHeaderString(conn, "Cache-Control", "no-cache");
    httpCreateCGIParams(conn);
    espRender(conn, "\r\n");

    /*
        Query
     */
    for (ITERATE_JSON(rx->params, jkey, i)) {
        espRender(conn, "PARAMS %s=%s\r\n", jkey->name, jkey->value ? jkey->value : "null");
    }
    espRender(conn, "\r\n");

    /*
        Http Headers
     */
    env = espGetHeaderHash(conn);
    for (ITERATE_KEYS(env, kp)) {
        espRender(conn, "HEADER %s=%s\r\n", kp->key, kp->data ? kp->data: "null");
    }
    espRender(conn, "\r\n");

    /*
        Server vars
     */
    for (ITERATE_KEYS(conn->rx->svars, kp)) {
        espRender(conn, "SERVER %s=%s\r\n", kp->key, kp->data ? kp->data: "null");
    }
    espRender(conn, "\r\n");

    /*
        Form vars
     */
    if ((params = espGetParams(conn)) != 0) {
        for (ITERATE_JSON(params, param, i)) {
            espRender(conn, "FORM %s=%s\r\n", param->name, param->value);
        }
        espRender(conn, "\r\n");
    }

#if KEEP
    /*
        Body
     */
    q = conn->readq;
    if (q->first && rx->bytesRead > 0 && scmp(rx->mimeType, "application/x-www-form-urlencoded") == 0) {
        buf = q->first->content;
        mprAddNullToBuf(buf);
        if ((numKeys = getParams(&keys, mprGetBufStart(buf), (int) mprGetBufLength(buf))) > 0) {
            for (i = 0; i < (numKeys * 2); i += 2) {
                value = keys[i+1];
                espRender(conn, "BODY %s=%s\r\n", keys[i], value ? value: "null");
            }
        }
        espRender(conn, "\r\n");
    }
#endif
}


PUBLIC bool espTestConfig(HttpRoute *route, cchar *key, cchar *desired)
{
    EspRoute    *eroute;
    cchar       *value;

    eroute = route->eroute;
    if ((value = mprGetJson(eroute->config, key)) != 0) {
        return smatch(value, desired);
    }
    return 0;
}


PUBLIC void espUpdateCache(HttpConn *conn, cchar *uri, cchar *data, int lifesecs)
{
    httpUpdateCache(conn, uri, data, lifesecs * MPR_TICKS_PER_SEC);
}


PUBLIC cchar *espUri(HttpConn *conn, cchar *target)
{
    return httpLink(conn, target);
}


PUBLIC int espEmail(HttpConn *conn, cchar *to, cchar *from, cchar *subject, MprTime date, cchar *mime, cchar *message, MprList *files)
{
    MprList         *lines;
    MprCmd          *cmd;
    cchar           *body, *boundary, *contents, *encoded, *file;
    char            *out, *err;
    ssize           length;
    int             i, next, status;

    if (!from || !*from) {
        from = "anonymous";
    }
    if (!subject || !*subject) {
        subject = "Mail message";
    }
    if (!mime || !*mime) {
        mime = "text/plain";
    }
    if (!date) {
        date = mprGetTime();
    }
    boundary = sjoin("esp.mail=", mprGetMD5("BOUNDARY"), NULL);
    lines = mprCreateList(0, 0);

    mprAddItem(lines, sfmt("To: %s", to));
    mprAddItem(lines, sfmt("From: %s", from));
    mprAddItem(lines, sfmt("Date: %s", mprFormatLocalTime(0, date)));
    mprAddItem(lines, sfmt("Subject: %s", subject));
    mprAddItem(lines, "MIME-Version: 1.0");
    mprAddItem(lines, sfmt("Content-Type: multipart/mixed; boundary=%s", boundary));
    mprAddItem(lines, "");

    boundary = sjoin("--", boundary, NULL);

    mprAddItem(lines, boundary);
    mprAddItem(lines, sfmt("Content-Type: %s", mime));
    mprAddItem(lines, "");
    mprAddItem(lines, "");
    mprAddItem(lines, message);

    for (ITERATE_ITEMS(files, file, next)) {
        mprAddItem(lines, boundary);
        if ((mime = mprLookupMime(NULL, file)) == 0) {
            mime = "application/octet-stream";
        }
        mprAddItem(lines, "Content-Transfer-Encoding: base64");
        mprAddItem(lines, sfmt("Content-Disposition: inline; filename=\"%s\"", mprGetPathBase(file)));
        mprAddItem(lines, sfmt("Content-Type: %s; name=\"%s\"", mime, mprGetPathBase(file)));
        mprAddItem(lines, "");
        contents = mprReadPathContents(file, &length);
        encoded = mprEncode64Block(contents, length);
        for (i = 0; i < length; i += 76) {
            mprAddItem(lines, snclone(&encoded[i], i + 76));
        }
    }
    mprAddItem(lines, sfmt("%s--", boundary));

    body = mprListToString(lines, "\n");
    mprTrace(4, "Password reset message:\n%s\n", body);

    cmd = mprCreateCmd(conn->dispatcher);
    if (mprRunCmd(cmd, "sendmail -t", NULL, body, &out, &err, 0, 0) < 0) {
        return MPR_ERR_CANT_OPEN;
    }
    if (mprWaitForCmd(cmd, ME_ESP_EMAIL_TIMEOUT) < 0) {
        mprError("Timeout %d msec waiting for command to complete: %s", ME_ESP_EMAIL_TIMEOUT, cmd->argv[0]);
        return MPR_ERR_CANT_COMPLETE;
    }
    if ((status = mprGetCmdExitStatus(cmd)) != 0) {
        mprError("Send mail failed status %d, error %s", status, err);
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}
   

PUBLIC void espClearCurrentSession(HttpConn *conn)
{
    EspRoute    *eroute;

    eroute = conn->rx->route->eroute;
    if (eroute->currentSession) {
        mprLog(4, "esp: clear current session %s", eroute->currentSession);
    }
    eroute->currentSession = 0;
}


/*
    Remember this connections session as the current session. Use for single login tracking
 */
PUBLIC void espSetCurrentSession(HttpConn *conn)
{
    EspRoute    *eroute;

    eroute = conn->rx->route->eroute;
    eroute->currentSession = httpGetSessionID(conn);
    mprLog(4, "esp: set current session %s", eroute->currentSession);
}


/*
    Test if this connection is the current session. Use for single login tracking.
 */
PUBLIC bool espIsCurrentSession(HttpConn *conn)
{
    EspRoute    *eroute;

    eroute = conn->rx->route->eroute;
    if (eroute->currentSession) {
        if (smatch(httpGetSessionID(conn), eroute->currentSession)) {
            return 1;
        }
        if (httpLookupSessionID(eroute->currentSession)) {
            /* Session is still current */
            return 0;
        }
        /* Session has expired */
        eroute->currentSession = 0;
    }
    return 1;
}

#endif /* ME_COM_ESP */
/*
    @copy   default

    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */

/************************************************************************/
/*
    Start of file "src/espHandler.c"
 */
/************************************************************************/
/*
    espHandler.c -- Embedded Server Pages (ESP) handler

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#if ME_COM_ESP

/************************************* Local **********************************/
/*
    Singleton ESP control structure
 */
static Esp *esp;

/************************************ Forward *********************************/

PUBLIC EspRoute *espInitRoute(HttpRoute *route);
static int espDbDirective(MaState *state, cchar *key, cchar *value);
static int espEnvDirective(MaState *state, cchar *key, cchar *value);
static EspRoute *getEroute(HttpRoute *route);
static void manageEsp(Esp *esp, int flags);
static void manageReq(EspReq *req, int flags);
static int runAction(HttpConn *conn);
static int unloadEsp(MprModule *mp);
static bool pageExists(HttpConn *conn);

#if !ME_STATIC
static char *getModuleEntry(EspRoute *eroute, cchar *kind, cchar *source, cchar *cacheName);
static bool layoutIsStale(EspRoute *eroute, cchar *source, cchar *module);
static bool loadApp(HttpRoute *route);
static bool loadEspModule(HttpRoute *route, cchar *kind, cchar *source, cchar **errMsg);
#endif

/************************************* Code ***********************************/
/*
    Open an instance of the ESP for a new request
 */
static void openEsp(HttpQueue *q)
{
    HttpConn    *conn;
    HttpRx      *rx;
    HttpRoute   *route;
    EspRoute    *eroute;
    EspReq      *req;

    conn = q->conn;
    rx = conn->rx;

    /*
        If unloading a module, this lock will cause a wait here while ESP applications are reloaded.
     */
    lock(esp);
    esp->inUse++;
    unlock(esp);

    if ((req = mprAllocObj(EspReq, manageReq)) == 0) {
        httpMemoryError(conn);
        return;
    }
    /*
        Find the ESP route configuration. Search up the route parent chain.
     */
    for (eroute = 0, route = rx->route; route; route = route->parent) {
        if (route->eroute) {
            eroute = route->eroute;
            break;
        }
    }
    if (!route) {
        route = rx->route;
        eroute = espInitRoute(route);
        if (maParseFile(NULL, mprJoinPath(mprGetAppDir(), "esp.conf")) < 0) {
            httpError(conn, 0, "Cannot parse esp.conf");
            return;
        }
    }
    if (!eroute) {
        httpError(conn, 0, "Cannot find a suitable ESP route");
        return;
    }
    conn->data = req;
    req->esp = esp;
    req->route = route;
    req->autoFinalize = 1;
    /*
        If a cookie is not explicitly set, use the application name for the session cookie
     */
    if (!route->cookie && eroute->appName && *eroute->appName) {
        route->cookie = eroute->appName;
    }
}


static void closeEsp(HttpQueue *q)
{
    lock(esp);
    esp->inUse--;
    assert(esp->inUse >= 0);
    unlock(esp);
}


#if !ME_STATIC
/*
    This is called when unloading a view or controller module
    All of ESP must be quiesced.
 */
static bool espUnloadModule(cchar *module, MprTicks timeout)
{
    MprModule   *mp;
    MprTicks    mark;
    Esp         *esp;

    if ((mp = mprLookupModule(module)) != 0) {
        esp = MPR->espService;
        mark = mprGetTicks();
        esp->reloading = 1;
        do {
            lock(esp);
            /* Own request will count as 1 */
            if (esp->inUse <= 1) {
                if (mprUnloadModule(mp) < 0) {
                    mprError("Cannot unload module %s", mp->name);
                    unlock(esp);
                }
                esp->reloading = 0;
                unlock(esp);
                return 1;
            }
            unlock(esp);
            mprSleep(10);

        } while (mprGetRemainingTicks(mark, timeout) > 0);
        esp->reloading = 0;
    }
    return 0;
}
#endif


PUBLIC void espClearFlash(HttpConn *conn)
{
    EspReq      *req;

    req = conn->data;
    req->flash = 0;
}


static void setupFlash(HttpConn *conn)
{
    EspReq      *req;

    req = conn->data;
    if (httpGetSession(conn, 0)) {
        req->flash = httpGetSessionObj(conn, ESP_FLASH_VAR);
        req->lastFlash = 0;
        if (req->flash) {
            httpRemoveSessionVar(conn, ESP_FLASH_VAR);
            req->lastFlash = mprCloneHash(req->flash);
        }
    }
}


static void pruneFlash(HttpConn *conn)
{
    EspReq  *req;
    MprKey  *kp, *lp;

    req = conn->data;
    if (req->flash && req->lastFlash) {
        for (ITERATE_KEYS(req->flash, kp)) {
            for (ITERATE_KEYS(req->lastFlash, lp)) {
                if (smatch(kp->key, lp->key)) {
                    mprRemoveKey(req->flash, kp->key);
                }
            }
        }
    }
}


static void finalizeFlash(HttpConn *conn)
{
    EspReq  *req;

    req = conn->data;
    if (req->flash && mprGetHashLength(req->flash) > 0) {
        /*  
            If the session does not exist, this will create one. However, must not have
            emitted the headers, otherwise cannot inform the client of the session cookie.
        */
        httpSetSessionObj(conn, ESP_FLASH_VAR, req->flash);
    }
}


/*
    Start the request. At this stage, body data may not have been fully received unless 
    the request is a form (POST method and Content-Type is application/x-www-form-urlencoded).
    Forms are a special case and delay invoking the start callback until all body data is received.
    WARNING: GC yield
 */
static void startEsp(HttpQueue *q)
{
    HttpConn    *conn;
    HttpRoute   *route;
    EspRoute    *eroute;
    EspReq      *req;
    cchar       *view;

    conn = q->conn;
    route = conn->rx->route;
    eroute = route->eroute;
    req = conn->data;

    if (req) {
        mprSetThreadData(req->esp->local, conn);
        httpAuthenticate(conn);
        setupFlash(conn);
        /*
            See if the esp configuration or app needs to be reloaded.
         */
        if (eroute->appName && espLoadConfig(route) < 0) {
            httpError(conn, HTTP_CODE_NOT_FOUND, "Cannot load esp config for %s", eroute->appName);
            return;
        }
#if !ME_STATIC
        /* WARNING: GC yield */
        if (!loadApp(route)) {
            httpError(conn, HTTP_CODE_NOT_FOUND, "Cannot load esp module for %s", eroute->appName);
            return;
        }
#endif
        /* WARNING: GC yield */
        if (!runAction(conn)) {
            pruneFlash(conn);
        } else {
            if (req->autoFinalize) {
                if (!conn->tx->responded) {
                    view = (route->sourceName && route->sourceName) ? conn->rx->target : NULL;
                    /* WARNING: GC yield */
                    espRenderView(conn, view);
                }
                if (req->autoFinalize) {
                    espFinalize(conn);
                }
            }
            pruneFlash(conn);
        }
        finalizeFlash(conn);
        mprSetThreadData(req->esp->local, NULL);
    }
}


/*
    Create a per user session database clone. Used for demos so one users updates to not change anothers view of the database
 */
static void pruneDatabases(Esp *esp)
{
    MprKey      *kp;

    lock(esp);
    for (ITERATE_KEYS(esp->databases, kp)) {
        if (!httpLookupSessionID(kp->key)) {
            mprRemoveKey(esp->databases, kp->key);
            /* Restart scan */
            kp = 0;
        }
    }
    unlock(esp);
}

static int cloneDatabase(HttpConn *conn)
{
    Esp         *esp;
    EspRoute    *eroute;
    EspReq      *req;
    cchar       *id;

    req = conn->data;
    eroute = conn->rx->route->eroute;
    assert(eroute->edi);
    assert(eroute->edi->flags & EDI_PRIVATE);

    esp = req->esp;
    if (!esp->databases) {
        lock(esp);
        if (!esp->databases) {
            esp->databases = mprCreateHash(0, 0);
            esp->databasesTimer = mprCreateTimerEvent(NULL, "esp-databases", 60 * 1000, pruneDatabases, esp, 0);
        }
        unlock(esp);
    }
    httpGetSession(conn, 1);
    id = httpGetSessionID(conn);
    if ((req->edi = mprLookupKey(esp->databases, id)) == 0) {
        if ((req->edi = ediClone(eroute->edi)) == 0) {
            mprError("Cannot clone database: %s", eroute->edi->path);
            return MPR_ERR_CANT_OPEN;
        }
        mprAddKey(esp->databases, id, req->edi);
    }
    return 0;
}



static int runAction(HttpConn *conn)
{
    HttpRx      *rx;
    HttpRoute   *route;
    EspRoute    *eroute;
    EspReq      *req;
    EspAction   action;
    char        *actionName, *key, *filename, *source;

    rx = conn->rx;
    req = conn->data;
    route = rx->route;
    eroute = route->eroute;
    assert(eroute);

    if (eroute->edi && eroute->edi->flags & EDI_PRIVATE) {
        cloneDatabase(conn);
    } else {
        req->edi = eroute->edi;
    }
    if (route->sourceName == 0 || *route->sourceName == '\0') {
        if (eroute->commonController) {
            (eroute->commonController)(conn);
        }
        return 1;
    }
    /*
        Expand any form var $tokens. This permits ${controller} and user form data to be used in the controller name
     */
    filename = schr(route->sourceName, '$') ? stemplateJson(route->sourceName, rx->params) : route->sourceName;
    source = eroute->controllersDir ? mprJoinPath(eroute->controllersDir, filename) : mprJoinPath(route->home, filename);

#if !ME_STATIC
    key = mprJoinPath(eroute->controllersDir, rx->target);
    if (!eroute->combined && (eroute->update || !mprLookupKey(esp->actions, key))) {
        cchar *errMsg;
        if (!loadEspModule(route, "controller", source, &errMsg)) {
            httpError(conn, HTTP_CODE_NOT_FOUND, "%s", errMsg);
            return 0;
        }
    }
#endif /* !ME_STATIC */
    key = mprJoinPath(eroute->controllersDir, rx->target);
    if ((action = mprLookupKey(esp->actions, key)) == 0) {
        if (!pageExists(conn)) {
            /*
                Actions are registered as: source/TARGET where TARGET is typically CONTROLLER-ACTION
             */
            key = sfmt("%s/missing", mprGetPathDir(source));
            if ((action = mprLookupKey(esp->actions, key)) == 0) {
                if ((action = mprLookupKey(esp->actions, "missing")) == 0) {
                    httpError(conn, HTTP_CODE_NOT_FOUND, "Missing action for %s in %s", rx->target, source);
                    return 0;
                }
            }
        }
    }
    if (route->flags & HTTP_ROUTE_XSRF) {
        if (!httpCheckSecurityToken(conn)) {
            if (rx->flags & HTTP_POST) {
                httpSetStatus(conn, HTTP_CODE_UNAUTHORIZED);
                if (eroute->json) {
                    mprLog(2, "esp: Stale security token.");
                    espRenderString(conn, "{\"retry\": true, \"success\": 0, \"feedback\": {\"error\": \"Security token is stale. Please retry.\"}}");
                    espFinalize(conn);
                } else {
                    httpError(conn, HTTP_CODE_UNAUTHORIZED, "Security token is stale. Please reload page.");
                }
                return 0;
            }
        }
    }
    if (action) {
        httpSetParam(conn, "controller", stok(sclone(rx->target), "-", &actionName));
        httpSetParam(conn, "action", actionName);
        if (eroute->commonController) {
            (eroute->commonController)(conn);
        }
        if (!httpIsFinalized(conn)) {
            (action)(conn);
        }
    }
    return 1;
}


PUBLIC void espRenderView(HttpConn *conn, cchar *name)
{
    HttpRx      *rx;
    HttpRoute   *route;
    EspRoute    *eroute;
    EspViewProc viewProc;
    cchar       *source;
    
    rx = conn->rx;
    route = rx->route;
    eroute = route->eroute;
    
    if (name) {
        source = mprJoinPathExt(mprJoinPath(eroute->viewsDir, name), ".esp");
    } else {
        httpMapFile(conn);
        source = conn->tx->filename;
    }
#if !ME_STATIC
    if (!eroute->combined && (eroute->update || !mprLookupKey(esp->views, mprGetPortablePath(source)))) {
        cchar *errMsg;
        /* WARNING: GC yield */
        mprHold(source);
        if (!loadEspModule(route, "view", source, &errMsg)) {
            mprRelease(source);
            httpError(conn, HTTP_CODE_NOT_FOUND, "%s", errMsg);
            return;
        }
        mprRelease(source);
    }
#endif
    if ((viewProc = mprLookupKey(esp->views, mprGetPortablePath(source))) == 0) {
        httpError(conn, HTTP_CODE_NOT_FOUND, "Cannot find view");
        return;
    }
    httpAddHeaderString(conn, "Content-Type", "text/html");
    if (rx->route->flags & HTTP_ROUTE_XSRF) {
        /* Add a new unique security token */
        httpAddSecurityToken(conn, 1);
    }
    /* WARNING: GC yield */
    (viewProc)(conn);
}


/************************************ Support *********************************/

#if !ME_STATIC
static char *getModuleEntry(EspRoute *eroute, cchar *kind, cchar *source, cchar *cacheName)
{
    char    *cp, *entry;
    
    if (smatch(kind, "view")) {
        entry = sfmt("esp_%s", cacheName);

    } else if (smatch(kind, "app")) {
        if (eroute->combined) {
            entry = sfmt("esp_%s_%s_combined", kind, eroute->appName);
        } else {
            entry = sfmt("esp_%s_%s", kind, eroute->appName);
        }
    } else {
        /* Controller */
        if (eroute->appName) {
            entry = sfmt("esp_%s_%s_%s", kind, eroute->appName, mprTrimPathExt(mprGetPathBase(source)));
        } else {
            entry = sfmt("esp_%s_%s", kind, mprTrimPathExt(mprGetPathBase(source)));
        }
    }
    for (cp = entry; *cp; cp++) {
        if (!isalnum((uchar) *cp) && *cp != '_') {
            *cp = '_';
        }
    }
    return entry;
}


/*
    WARNING: GC yield
 */
static bool loadEspModule(HttpRoute *route, cchar *kind, cchar *source, cchar **errMsg)
{
    Esp         *esp;
    EspRoute    *eroute;
    MprModule   *mp;
    cchar       *appName, *cacheName, *canonical, *entry, *module;
    int         isView, recompile;

    eroute = route->eroute;
    esp = MPR->espService;
    *errMsg = "";

#if VXWORKS
    /* 
        Trim the drive for VxWorks where simulated host drives only exist on the target
     */
    source = mprTrimPathDrive(source);
#endif
    canonical = mprGetPortablePath(mprGetRelPath(source, route->documents));
    appName = eroute->appName ? eroute->appName : route->host->name;
    if (eroute->combined) {
        cacheName = eroute->appName;
    } else {
        cacheName = mprGetMD5WithPrefix(sfmt("%s:%s", appName, canonical), -1, sjoin(kind, "_", NULL));
    }
    module = mprNormalizePath(sfmt("%s/%s%s", eroute->cacheDir, cacheName, ME_SHOBJ));
    isView = smatch(kind, "view");

    lock(esp);
    if (eroute->update) {
        if (!mprPathExists(source, R_OK)) {
            *errMsg = sfmt("Cannot find %s \"%s\" to load", kind, source);
            unlock(esp);
            return 0;
        }
        if (espModuleIsStale(source, module, &recompile) || (isView && layoutIsStale(eroute, source, module))) {
            if (recompile) {
                mprHoldBlocks(source, module, cacheName, NULL);
                if (!espCompile(route, NULL, source, module, cacheName, isView, (char**) errMsg)) {
                    mprReleaseBlocks(source, module, cacheName, NULL);
                    unlock(esp);
                    return 0;
                }
                mprReleaseBlocks(source, module, cacheName, NULL);
            }
        }
    } else {
        mprTrace(4, "EspUpdate is disabled for this route: %s", route->name);
    }
    if (mprLookupModule(source) == 0) {
        entry = getModuleEntry(eroute, kind, source, cacheName);
        if ((mp = mprCreateModule(source, module, entry, route)) == 0) {
            *errMsg = "Memory allocation error loading module";
            unlock(esp);
            return 0;
        }
        mprLog(3, "esp: loadEspModule: \"%s\", %s", kind, source);
        if (mprLoadModule(mp) < 0) {
            *errMsg = "Cannot load compiled esp module";
            unlock(esp);
            return 0;
        }
    }
    unlock(esp);
    return 1;
}


/* 
    WARNING: GC yield 
 */
static bool loadApp(HttpRoute *route)
{
    EspRoute    *eroute;
    cchar       *source, *errMsg;

    eroute = route->eroute;
    if (!eroute->appName) {
        return 1;
    }
    if (eroute->loaded && !eroute->update) {
        return 1;
    }
    if (eroute->combined) {
        source = mprJoinPath(eroute->cacheDir, sfmt("%s.c", eroute->appName));
    } else {
        source = mprJoinPath(eroute->srcDir, "app.c");
    }
    if (mprPathExists(source, R_OK)) {
        if (!loadEspModule(route, "app", source, &errMsg)) {
            mprError("%s", errMsg);
            return 0;
        }
    }
    eroute->loaded = 1;
    return 1;
}


/*
    Test if a module has been updated (is stale).
    This will unload the module if it loaded but stale.
    Set recompile to true if the source is absent or more recent
 */
PUBLIC bool espModuleIsStale(cchar *source, cchar *module, int *recompile)
{
    MprModule   *mp;
    MprPath     sinfo, minfo;

    *recompile = 0;
    mprGetPathInfo(module, &minfo);
    if (!minfo.valid) {
        if ((mp = mprLookupModule(source)) != 0) {
            if (!espUnloadModule(source, ME_ESP_RELOAD_TIMEOUT)) {
                mprError("Cannot unload module %s. Connections still open. Continue using old version.", source);
                return 0;
            }
        }
        *recompile = 1;
        mprLog(4, "esp: %s is newer than module %s, recompiling ...", source, module);
        return 1;
    }
    mprGetPathInfo(source, &sinfo);
    if (sinfo.valid && sinfo.mtime > minfo.mtime) {
        if ((mp = mprLookupModule(source)) != 0) {
            if (!espUnloadModule(source, ME_ESP_RELOAD_TIMEOUT)) {
                mprError("Cannot unload module %s. Connections still open. Continue using old version.", source);
                return 0;
            }
        }
        *recompile = 1;
        mprLog(4, "esp: %s is newer than module %s, recompiling ...", source, module);
        return 1;
    }
    if ((mp = mprLookupModule(source)) != 0) {
        if (minfo.mtime > mp->modified) {
            /* Module file has been updated */
            if (!espUnloadModule(source, ME_ESP_RELOAD_TIMEOUT)) {
                mprError("Cannot unload module %s. Connections still open. Continue using old version.", source);
                return 0;
            }
            mprLog(4, "esp: module %s has been externally updated, reloading ...", module);
            return 1;
        }
    }
    /* Loaded module is current */
    return 0;
}


/*
    Check if the layout has changed
 */
static bool layoutIsStale(EspRoute *eroute, cchar *source, cchar *module)
{
    char    *data, *lpath, *quote;
    cchar   *layout;
    ssize   len;
    bool    stale;
    int     recompile;

    stale = 0;
    if ((data = mprReadPathContents(source, &len)) != 0) {
        if ((lpath = scontains(data, "@ layout \"")) != 0) {
            lpath = strim(&lpath[10], " ", MPR_TRIM_BOTH);
            if ((quote = schr(lpath, '"')) != 0) {
                *quote = '\0';
            }
            layout = (eroute->layoutsDir && *lpath) ? mprJoinPath(eroute->layoutsDir, lpath) : 0;
        } else {
            layout = (eroute->layoutsDir) ? mprJoinPath(eroute->layoutsDir, "default.esp") : 0;
        }
        if (layout) {
            stale = espModuleIsStale(layout, module, &recompile);
            if (stale) {
                mprLog(4, "esp: layout %s is newer than module %s", layout, module);
            }
        }
    }
    return stale;
}
#else

PUBLIC bool espModuleIsStale(cchar *source, cchar *module, int *recompile)
{
    return 0;
}
#endif /* ME_STATIC */


/*
    Test if the the required ESP page exists
 */
static bool pageExists(HttpConn *conn)
{
    HttpRx      *rx;
    EspRoute    *eroute;
    cchar       *source, *dir;
    
    rx = conn->rx;
    eroute = rx->route->eroute;
    dir = eroute->viewsDir ? eroute->viewsDir : rx->route->documents;
    source = mprJoinPathExt(mprJoinPath(dir, rx->target), ".esp");
    return mprPathExists(source, R_OK);
}


/************************************ Esp Route *******************************/
/*
    Public so that esp.c can also call
 */
PUBLIC void espManageEspRoute(EspRoute *eroute, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(eroute->appDir);
        mprMark(eroute->appName);
        mprMark(eroute->cacheDir);
        mprMark(eroute->client);
        mprMark(eroute->clientDir);
        mprMark(eroute->compile);
        mprMark(eroute->config);
        mprMark(eroute->controllersDir);
        mprMark(eroute->currentSession);
        mprMark(eroute->database);
        mprMark(eroute->dbDir);
        mprMark(eroute->edi);
        mprMark(eroute->env);
        mprMark(eroute->layoutsDir);
        mprMark(eroute->libDir);
        mprMark(eroute->link);
        mprMark(eroute->mutex);
        mprMark(eroute->searchPath);
        mprMark(eroute->routeSet);
        mprMark(eroute->srcDir);
        mprMark(eroute->viewsDir);
        mprMark(eroute->winsdk);
        mprMark(eroute->top);
    }
}


PUBLIC EspRoute *espInitRoute(HttpRoute *route)
{
    cchar       *path;
    MprPath     info;
    EspRoute    *eroute;
    
    if (route->eroute) {
        eroute = route->eroute;
    } else if ((eroute = mprAllocObj(EspRoute, espManageEspRoute)) == 0) {
        return 0;
    }
#if DEBUG_IDE && KEEP
    path = mprGetAppDir();
#else
    path = httpGetRouteVar(route, "CACHE_DIR");
    if (!path) {
        path = mprJoinPath(route->home, "cache");
        mprMakeDir(path, 0755, -1, -1, 1);
    }
    if (mprGetPathInfo(path, &info) != 0 || !info.isDir) {
        path = route->home;
    }
#endif
    /*
        Use a relative path incase a Chroot directive happens after loading the esp handler
     */
    eroute->cacheDir = (char*) mprGetRelPath(path, NULL);
    eroute->update = ME_DEBUG;
    eroute->keepSource = ME_DEBUG;
    eroute->lifespan = 0;
    eroute->mutex = mprCreateLock();
    route->eroute = eroute;
    return eroute;
}


static EspRoute *cloneEspRoute(HttpRoute *route, EspRoute *parent)
{
    EspRoute      *eroute;
    
    assert(parent);
    assert(route);

    if ((eroute = mprAllocObj(EspRoute, espManageEspRoute)) == 0) {
        return 0;
    }
    eroute->top = parent->top;
    eroute->searchPath = parent->searchPath;
    eroute->edi = parent->edi;
    eroute->commonController = parent->commonController;
    eroute->update = parent->update;
    eroute->keepSource = parent->keepSource;
    eroute->lifespan = parent->lifespan;
    if (parent->compile) {
        eroute->compile = sclone(parent->compile);
    }
    if (parent->link) {
        eroute->link = sclone(parent->link);
    }
    if (parent->env) {
        eroute->env = mprCloneHash(parent->env);
    }
    eroute->appDir = parent->appDir;
    eroute->appName = parent->appName;
    eroute->cacheDir = parent->cacheDir;
    eroute->clientDir = parent->clientDir;
    eroute->config = parent->config;
    eroute->configLoaded = parent->configLoaded;
    eroute->dbDir = parent->dbDir;
    eroute->layoutsDir = parent->layoutsDir;
    eroute->libDir = parent->libDir;
    eroute->srcDir = parent->srcDir;
    eroute->controllersDir = parent->controllersDir;
    eroute->viewsDir = parent->viewsDir;
    route->eroute = eroute;
    return eroute;
}


PUBLIC void espSetDefaultDirs(HttpRoute *route)
{
    EspRoute    *eroute;
    char        *dir;

    eroute = route->eroute;
    dir = route->documents;

    eroute->dbDir       = mprJoinPath(dir, "db");
    eroute->cacheDir    = mprJoinPath(dir, "cache");
    eroute->clientDir   = mprJoinPath(dir, "client");
    eroute->controllersDir = mprJoinPath(dir, "controllers");
    eroute->srcDir      = mprJoinPath(dir, "src");
    eroute->appDir      = mprJoinPath(dir, "client/app");
    eroute->layoutsDir  = mprJoinPath(eroute->clientDir, "layouts");
    eroute->libDir      = mprJoinPath(eroute->clientDir, "lib");
    eroute->viewsDir    = eroute->appDir;

    httpSetRouteVar(route, "CACHE_DIR", eroute->cacheDir);
    httpSetRouteVar(route, "CLIENT_DIR", eroute->clientDir);
    httpSetRouteVar(route, "CONTROLLERS_DIR", eroute->controllersDir);
    httpSetRouteVar(route, "DB_DIR", eroute->dbDir);
    httpSetRouteVar(route, "LAYOUTS_DIR", eroute->layoutsDir);
    httpSetRouteVar(route, "LIB_DIR", eroute->libDir);
    httpSetRouteVar(route, "SRC_DIR", eroute->srcDir);
    httpSetRouteVar(route, "VIEWS_DIR", eroute->viewsDir);
}


#if ME_ESP_LEGACY
PUBLIC void espSetLegacyDirs(HttpRoute *route)
{
    EspRoute    *eroute;
    char        *dir;

    eroute = route->eroute;
    dir = route->documents;
    eroute->clientDir  = mprJoinPath(dir, "static");
    eroute->layoutsDir = mprJoinPath(dir, "layouts");
    eroute->viewsDir   = mprJoinPath(dir, "views");
    httpSetRouteVar(route, "CLIENT_DIR", eroute->clientDir);
    httpSetRouteVar(route, "LAYOUTS_DIR", eroute->layoutsDir);
    httpSetRouteVar(route, "VIEWS_DIR", eroute->viewsDir);
}
#endif


/*
    Manage all links for EspReq for the garbage collector
 */
static void manageReq(EspReq *req, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(req->commandLine);
        mprMark(req->flash);
        mprMark(req->lastFlash);
        mprMark(req->feedback);
        mprMark(req->route);
        mprMark(req->data);
        mprMark(req->edi);
    }
}


/*
    Manage all links for Esp for the garbage collector
 */
static void manageEsp(Esp *esp, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(esp->actions);
        mprMark(esp->databases);
        mprMark(esp->databasesTimer);
        mprMark(esp->ediService);
#if ME_ESP_LEGACY
        mprMark(esp->internalOptions);
#endif
        mprMark(esp->local);
        mprMark(esp->mutex);
        mprMark(esp->views);
    }
}


/*
    Get a dedicated EspRoute for an HttpRoute. Allocate if required. It is expected that the caller will modify the EspRoute.
 */
static EspRoute *getEroute(HttpRoute *route)
{
    HttpRoute   *rp;

    if (route->eroute) {
        if (!route->parent || route->parent->eroute != route->eroute) {
            return route->eroute;
        }
    }
    /*
        Lookup up the route chain for any configured EspRoutes
     */
    for (rp = route; rp; rp = rp->parent) {
        if (rp->eroute) {
            return cloneEspRoute(route, rp->eroute);
        }
    }
    return espInitRoute(route);
}


#if ME_ESP_LEGACY
/*
    Deprecated in 4.4
 */
static HttpRoute *addLegacyRestful(HttpRoute *parent, cchar *prefix, cchar *action, cchar *methods, cchar *pattern, 
    cchar *target, cchar *resource)
{
    cchar       *name, *nameResource, *source, *token;

    token = "{controller}";
    nameResource = smatch(resource, token) ? "*" : resource;

    if (parent->prefix) {
        name = sfmt("%s/%s/%s", parent->prefix, nameResource, action);
        pattern = sfmt("^%s/%s%s", parent->prefix, resource, pattern);
    } else {
        name = sfmt("/%s/%s", nameResource, action);
        if (*resource == '{') {
            pattern = sfmt("^/%s%s", resource, pattern);
        } else {
            pattern = sfmt("^/{controller=%s}%s", resource, pattern);
        }
    }
    if (*resource == '{') {
        target = sfmt("$%s-%s", resource, target);
        source = sfmt("$%s.c", resource);
    } else {
        target = sfmt("%s-%s", resource, target);
        source = sfmt("%s.c", resource);
    }
    return httpDefineRoute(parent, name, methods, pattern, target, source);
}


/*
    Deprecated in 4.4.0
 */
PUBLIC void httpAddLegacyResourceGroup(HttpRoute *parent, cchar *prefix, cchar *resource)
{
    addLegacyRestful(parent, prefix, "create",    "POST",    "(/)*$",                   "create",        resource);
    addLegacyRestful(parent, prefix, "destroy",   "DELETE",  "/{id=[0-9]+}$",           "destroy",       resource);
    addLegacyRestful(parent, prefix, "edit",      "GET",     "/{id=[0-9]+}/edit$",      "edit",          resource);
    addLegacyRestful(parent, prefix, "init",      "GET",     "/init$",                  "init",          resource);
    addLegacyRestful(parent, prefix, "list",      "GET",     "(/)*$",                   "list",          resource);
    addLegacyRestful(parent, prefix, "show",      "GET",     "/{id=[0-9]+}$",           "show",          resource);
    addLegacyRestful(parent, prefix, "update",    "PUT",     "/{id=[0-9]+}$",           "update",        resource);
    addLegacyRestful(parent, prefix, "action",    "POST",    "/{action}/{id=[0-9]+}$",  "${action}",     resource);
    addLegacyRestful(parent, prefix, "default",   "GET,POST","/{action}$",              "cmd-${action}", resource);
}


PUBLIC void httpAddLegacyResource(HttpRoute *parent, cchar *prefix, cchar *resource)
{
    addLegacyRestful(parent, prefix, "init",      "GET",     "/init$",       "init",          resource);
    addLegacyRestful(parent, prefix, "create",    "POST",    "(/)*$",        "create",        resource);
    addLegacyRestful(parent, prefix, "edit",      "GET",     "/edit$",       "edit",          resource);
    addLegacyRestful(parent, prefix, "show",      "GET",     "(/)*$",        "show",          resource);
    addLegacyRestful(parent, prefix, "update",    "PUT",     "(/)*$",        "update",        resource);
    addLegacyRestful(parent, prefix, "destroy",   "DELETE",  "(/)*$",        "destroy",       resource);
    addLegacyRestful(parent, prefix, "default",   "GET,POST","/{action}$",   "cmd-${action}", resource);
}
#endif


PUBLIC void espAddHomeRoute(HttpRoute *parent)
{
    cchar   *source, *name, *path, *pattern, *prefix;

    prefix = parent->prefix ? parent->prefix : "";
    source = parent->sourceName;
    name = sjoin(prefix, "/home", NULL);
    path = stemplate("${CLIENT_DIR}/index.esp", parent->vars);
    pattern = sfmt("^%s(/)$", prefix);
    httpDefineRoute(parent, name, "GET,POST", pattern, path, source);
}


PUBLIC void espAddRouteSet(HttpRoute *route, cchar *set)
{
    EspRoute    *eroute;

    eroute = route->eroute;
    if (set == 0 || *set == 0) {
        return;
    }
    if (scaselessmatch(set, "esp-angular-mvc")) {
        httpAddWebSocketsRoute(route, route->serverPrefix, "/*/stream");
        httpAddResourceGroup(route, route->serverPrefix, "{controller}");
        httpAddClientRoute(route, "", "/public");
        httpHideRoute(route, 1);
        eroute->viewsDir = eroute->appDir;
        eroute->layoutsDir = mprJoinPath(route->home, "layouts");

    } else if (scaselessmatch(set, "esp-html-mvc")) {
        httpAddRestfulRoute(route, route->serverPrefix, "delete", "POST", "/{id=[0-9]+}/delete$", "delete", "{controller}");
        httpAddResourceGroup(route, route->serverPrefix, "{controller}");
        httpAddClientRoute(route, "", "/public");
        httpHideRoute(route, 1);
        eroute->viewsDir = eroute->appDir;
        eroute->layoutsDir = mprJoinPath(route->home, "layouts");

#if ME_ESP_LEGACY
    /*
        Deprecated in 4.4
     */
    } else if (scaselessmatch(set, "mvc")) {
        espAddHomeRoute(route);
        httpAddClientRoute(route, "/static", "/static");

    } else if (scaselessmatch(set, "mvc-fixed")) {
        espAddHomeRoute(route);
        httpAddClientRoute(route, "/static", "/static");
        httpDefineRoute(route, "default", NULL, "^/{controller}(~/{action}~)", "${controller}-${action}", "${controller}.c");

    } else if (scaselessmatch(set, "restful") || scaselessmatch(set, "esp-legacy-mvc")) {
        if (eroute->legacy) {
            espAddHomeRoute(route);
            httpAddClientRoute(route, "/static", "/static");
            httpAddLegacyResourceGroup(route, "", "{controller}");
        } else {
            httpAddResourceGroup(route, route->serverPrefix, "{controller}");
            httpAddClientRoute(route, "", "/client");
        }

    } else if (!scaselessmatch(set, "none")) {
        mprError("Unknown route set %s", set);
#endif
    }
}

/*********************************** Directives *******************************/
/*
    <EspApp 
  or 
    EspApp 
        auth=STORE 
        database=DATABASE 
        dir=DIR 
        combined=true|false
        name=NAME 
        prefix=PREFIX 
        routes=ROUTES 

    Old syntax DEPRECATED in 4.4: EspApp Prefix [Dir [RouteSet [Database]]]
 */
static int startEspAppDirective(MaState *state, cchar *key, cchar *value)
{
    HttpRoute   *route;
    EspRoute    *eroute;
    cchar       *auth, *database, *name, *prefix, *dir, *routeSet, *combined;
    char        *option, *ovalue, *tok;

    dir = ".";
    routeSet = 0;
    combined = "false";
    prefix = 0;
    database = 0;
    auth = 0;
    name = 0;

    if (scontains(value, "=")) {
        for (option = maGetNextArg(sclone(value), &tok); option; option = maGetNextArg(tok, &tok)) {
            option = stok(option, " =\t,", &ovalue);
            ovalue = strim(ovalue, "\"'", MPR_TRIM_BOTH);
            if (smatch(option, "auth")) {
                auth = ovalue;
            } else if (smatch(option, "database")) {
                database = ovalue;
            } else if (smatch(option, "dir")) {
                dir = ovalue;
            } else if (smatch(option, "combined")) {
                combined = ovalue;
#if DEPRECATED || 1
            } else if (smatch(option, "flat")) {
                combined = ovalue;
#endif
            } else if (smatch(option, "name")) {
                name = ovalue;
            } else if (smatch(option, "prefix")) {
                prefix = ovalue;
            } else if (smatch(option, "routes")) {
                routeSet = ovalue;
#if DEPRECATED || 1
            } else if (smatch(option, "type")) {
                /* Ignored */
#endif
            } else {
                mprError("Unknown EspApp option \"%s\"", option);
            }
        }

#if DEPRECATED || 1
    } else {
        /* 
            Deprecated in 4.4.0
         */
        if (!maTokenize(state, value, "%S ?S ?S ?S", &prefix, &dir, &routeSet, &database)) {
            return MPR_ERR_BAD_SYNTAX;
        }
        name = "app";
#endif
    }
    if (smatch(prefix, "/") || smatch(prefix, "")) {
        prefix = 0;
    }
    if (/* FUTURE smatch(prefix, state->route->prefix) && */ mprSamePath(state->route->documents, dir)) {
        /* 
            Can use existing route as it has the same prefix and documents directory. 
         */
        route = state->route;
    } else {
        route = httpCreateInheritedRoute(state->route);
    }
    state->route = route;
    httpSetRouteDocuments(route, dir);
    httpSetRouteHome(route, dir);

    if ((eroute = getEroute(route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    eroute->top = eroute;
    eroute->combined = scaselessmatch(combined, "true") || smatch(combined, "1");
    if (name) {
        eroute->appName = sclone(name);
    }
    httpSetRouteVar(route, "APP_NAME", name);
    eroute->routeSet = sclone(routeSet);
    espSetDefaultDirs(route);
    if (prefix) {
        if (*prefix != '/') {
            mprError("Prefix name should start with a \"/\"");
            prefix = sjoin("/", prefix, NULL);
        }
        prefix = stemplate(prefix, route->vars);
        httpSetRouteName(route, prefix);
        httpSetRoutePrefix(route, prefix);
        httpSetRoutePattern(route, sfmt("^%s", prefix), 0);
    } else {
        httpSetRouteName(route, sfmt("/%s", name));
    }
    httpSetRouteVar(route, "PREFIX", prefix);
    /*
        Load the config file here before creating routes
     */
    if (espLoadConfig(route) < 0) {
        return MPR_ERR_CANT_LOAD;
    }
    espSetConfig(route, "esp.appPrefix", prefix);
    espSetConfig(route, "esp.prefix", sjoin(prefix ? prefix : "", route->serverPrefix, NULL));
#if ME_ESP_LEGACY
    if (eroute->legacy && (eroute->routeSet == 0 || *eroute->routeSet == '\0')) {
        eroute->routeSet = sclone("restful");
    }
#endif
    if (auth) {
        if (httpSetAuthStore(route->auth, auth) < 0) {
            mprError("The %s AuthStore is not available on this platform", auth);
            return MPR_ERR_BAD_STATE;
        }
    }
    httpSetRouteTarget(route, "run", "$&");
    httpAddRouteHandler(route, "espHandler", "");
    httpAddRouteHandler(route, "espHandler", "esp");

#if ME_ESP_LEGACY
    if (!eroute->legacy)
#endif
        httpAddRouteIndex(route, "index.esp");

    if (database) {
        eroute->database = sclone(database);
        if (espDbDirective(state, key, eroute->database) < 0) {
            return MPR_ERR_BAD_STATE;
        }
    }
    return 0;
}


static int finishEspAppDirective(MaState *state, cchar *key, cchar *value)
{
    HttpRoute   *route;
    EspRoute    *eroute;

    /*
        The order of route finalization will be from the inside. Route finalization causes the route to be added
        to the enclosing host. This ensures that nested routes are defined BEFORE outer/enclosing routes.
     */
    route = state->route;
    eroute = route->eroute;
    espAddRouteSet(route, eroute->routeSet);
    if (route != state->prev->route) {
        httpFinalizeRoute(route);
    }
#if !ME_STATIC
    if (!state->appweb->skipModules) {
        MprJson *preload, *item;
        cchar   *errMsg, *source;
        char    *kind;
        int     i;
        /*
            Note: the config parser pauses GC, so this will never yield
         */
        if (!loadApp(route)) {
            return MPR_ERR_CANT_LOAD;
        }
        if (!eroute->combined && (preload = mprGetJsonObj(eroute->config, "app.esp.preload")) != 0) {
            for (ITERATE_JSON(preload, item, i)) {
                source = stok(sclone(item->value), ":", &kind);
                if (!kind) kind = "controller";
                source = mprJoinPath(eroute->controllersDir, source);
                if (!loadEspModule(state->route, kind, source, &errMsg)) {
                    mprError("Cannot preload esp module %s. %s", source, errMsg);
                    return MPR_ERR_CANT_LOAD;
                }
            }
        }
    }
#endif
    return 0;
}


/*
    <Espapp>
 */
static int openEspAppDirective(MaState *state, cchar *key, cchar *value)
{
    state = maPushState(state);
    return startEspAppDirective(state, key, value);
}


/*
    </Espapp>
 */
static int closeEspAppDirective(MaState *state, cchar *key, cchar *value)
{
    if (finishEspAppDirective(state, key, value) < 0) {
        return MPR_ERR_BAD_STATE;
    }
    maPopState(state);
    return 0;
}


/*
    see openEspAppDirective
 */
static int espAppDirective(MaState *state, cchar *key, cchar *value)
{
    state = maPushState(state);
    if (startEspAppDirective(state, key, value) < 0) {
        return MPR_ERR_BAD_STATE;
    }
    if (finishEspAppDirective(state, key, value) < 0) {
        return MPR_ERR_BAD_STATE;
    }
    maPopState(state);
    return 0;
}


/*
    EspCompile template
 */
static int espCompileDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;

    if ((eroute = getEroute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    eroute->compile = sclone(value);
    return 0;
}


PUBLIC int espOpenDatabase(HttpRoute *route, cchar *spec)
{
    EspRoute    *eroute;
    char        *provider, *path, *dir;
    int         flags;

    eroute = route->eroute;
    if (eroute->edi) {
        return 0;
    }
    flags = EDI_CREATE | EDI_AUTO_SAVE;
    if (smatch(spec, "default")) {
#if ME_COM_SQLITE
        spec = sfmt("sdb://%s.sdb", eroute->appName);
#elif ME_COM_MDB
        spec = sfmt("mdb://%s.mdb", eroute->appName);
#endif
    }
    provider = stok(sclone(spec), "://", &path);
    if (provider == 0 || path == 0) {
        return MPR_ERR_BAD_ARGS;
    }
    path = mprJoinPath(eroute->dbDir, path);
    dir = mprGetPathDir(path);
    if (!mprPathExists(dir, X_OK)) {
        mprMakeDir(dir, 0755, -1, -1, 1);
    }
    if ((eroute->edi = ediOpen(mprGetRelPath(path, NULL), provider, flags)) == 0) {
        return MPR_ERR_CANT_OPEN;
    }
    return 0;
}


/*
    EspDb provider://database
 */
static int espDbDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;

    if ((eroute = getEroute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (espOpenDatabase(state->route, value) < 0) {
        if (!(state->flags & MA_PARSE_NON_SERVER)) {
            mprError("Cannot open database '%s'. Use: provider://database", value);
            return MPR_ERR_CANT_OPEN;
        }
    }
    return 0;
}


/*
    EspDir key path
 */
static int espDirDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    char        *name, *path;

    if ((eroute = getEroute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%S ?S", &name, &path)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (smatch(name, "mvc")) {
        espSetDefaultDirs(state->route);
    } else {
        path = stemplate(path, state->route->vars);
        path = stemplate(mprJoinPath(state->route->home, path), state->route->vars);
        if (smatch(name, "cache")) {
            eroute->cacheDir = path;
        } else if (smatch(name, "client")) {
            eroute->clientDir = path;
        } else if (smatch(name, "controllers")) {
            eroute->controllersDir = path;
        } else if (smatch(name, "db")) {
            eroute->dbDir = path;
        } else if (smatch(name, "layouts")) {
            eroute->layoutsDir = path;
        } else if (smatch(name, "lib")) {
            eroute->libDir = path;
        } else if (smatch(name, "src")) {
            eroute->srcDir = path;
        } else if (smatch(name, "views")) {
            eroute->viewsDir = path;
#if DEPRECATED || 1
        } else if (smatch(name, "static")) {
            eroute->clientDir = path;
#endif
        }
        httpSetRouteVar(state->route, name, path);
    }
    return 0;
}


/*
    Define Visual Studio environment if not already present
 */
static void defineVisualStudioEnv(MaState *state)
{
    MaAppweb    *appweb;
    int         is64BitSystem;

    appweb = MPR->appwebService;

    if (scontains(getenv("LIB"), "Visual Studio") &&
        scontains(getenv("INCLUDE"), "Visual Studio") &&
        scontains(getenv("PATH"), "Visual Studio")) {
        return;
    }
    if (scontains(appweb->platform, "-x64-")) {
        is64BitSystem = smatch(getenv("PROCESSOR_ARCHITECTURE"), "AMD64") || getenv("PROCESSOR_ARCHITEW6432");
        espEnvDirective(state, "EspEnv", "LIB \"${WINSDK}\\LIB\\${WINVER}\\um\\x64;${WINSDK}\\LIB\\x64;${VS}\\VC\\lib\\amd64\"");
        if (is64BitSystem) {
            espEnvDirective(state, "EspEnv", 
                "PATH \"${VS}\\Common7\\IDE;${VS}\\VC\\bin\\amd64;${VS}\\Common7\\Tools;${VS}\\SDK\\v3.5\\bin;"
                "${VS}\\VC\\VCPackages;${WINSDK}\\bin\\x64\"");

        } else {
            /* Cross building on x86 for 64-bit */
            espEnvDirective(state, "EspEnv", 
                "PATH \"${VS}\\Common7\\IDE;${VS}\\VC\\bin\\x86_amd64;"
                "${VS}\\Common7\\Tools;${VS}\\SDK\\v3.5\\bin;${VS}\\VC\\VCPackages;${WINSDK}\\bin\\x86\"");
        }

    } else if (scontains(appweb->platform, "-arm-")) {
        /* Cross building on x86 for arm. No winsdk 7 support for arm */
        espEnvDirective(state, "EspEnv", "LIB \"${WINSDK}\\LIB\\${WINVER}\\um\\arm;${VS}\\VC\\lib\\arm\"");
        espEnvDirective(state, "EspEnv", "PATH \"${VS}\\Common7\\IDE;${VS}\\VC\\bin\\x86_arm;${VS}\\Common7\\Tools;"
            "${VS}\\SDK\\v3.5\\bin;${VS}\\VC\\VCPackages;${WINSDK}\\bin\\arm\"");

    } else {
        /* Building for X86 */
        espEnvDirective(state, "EspEnv", "LIB \"${WINSDK}\\LIB\\${WINVER}\\um\\x86;${WINSDK}\\LIB\\x86;"
            "${WINSDK}\\LIB;${VS}\\VC\\lib\"");
        espEnvDirective(state, "EspEnv", "PATH \"${VS}\\Common7\\IDE;${VS}\\VC\\bin;${VS}\\Common7\\Tools;"
            "${VS}\\SDK\\v3.5\\bin;${VS}\\VC\\VCPackages;${WINSDK}\\bin\"");
    }
    espEnvDirective(state, "EspEnv", "INCLUDE \"${VS}\\VC\\INCLUDE;${WINSDK}\\include;${WINSDK}\\include\\um;"
        "${WINSDK}\\include\\shared\"");
}


/*
    EspEnv var string
    This defines an environment variable setting. It is defined only when commands for this route are executed.
 */
static int espEnvDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    char        *ekey, *evalue;

    if ((eroute = getEroute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%S ?S", &ekey, &evalue)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (eroute->env == 0) {
        eroute->env = mprCreateHash(-1, MPR_HASH_STABLE);
    }
    evalue = espExpandCommand(state->route, evalue, "", "");
    if (scaselessmatch(ekey, "VisualStudio")) {
        defineVisualStudioEnv(state);
    } else {
        mprAddKey(eroute->env, ekey, evalue);
    }
    if (scaselessmatch(ekey, "PATH")) {
        if (eroute->searchPath) {
            eroute->searchPath = sclone(evalue);
        } else {
            eroute->searchPath = sjoin(eroute->searchPath, MPR_SEARCH_SEP, evalue, NULL);
        }
    }
    return 0;
}


/*
    EspKeepSource on|off
 */
static int espKeepSourceDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    bool        on;

    if ((eroute = getEroute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    eroute->keepSource = on;
    return 0;
}


/*
    EspLink template
 */
static int espLinkDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;

    if ((eroute = getEroute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    eroute->link = sclone(value);
    return 0;
}


/*
    Initialize and load a statically linked ESP module
 */
PUBLIC int espStaticInitialize(EspModuleEntry entry, cchar *appName, cchar *routeName)
{
    HttpRoute   *route;

    if ((route = httpLookupRoute(NULL, routeName)) == 0) {
        mprError("Cannot find route %s", routeName);
        return MPR_ERR_CANT_ACCESS;
    }
    return (entry)(route, NULL);
}


/*
    EspPermResource [resource ...]
 */
static int espPermResourceDirective(MaState *state, cchar *key, cchar *value)
{
    char        *name, *next;

    if (value == 0 || *value == '\0') {
        httpAddPermResource(state->route, state->route->serverPrefix, "{controller}");
    } else {
        name = stok(sclone(value), ", \t\r\n", &next);
        while (name) {
            httpAddPermResource(state->route, state->route->serverPrefix, name);
            name = stok(NULL, ", \t\r\n", &next);
        }
    }
    return 0;
}

/*
    EspResource [resource ...]
 */
static int espResourceDirective(MaState *state, cchar *key, cchar *value)
{
    char        *name, *next;

    if (value == 0 || *value == '\0') {
        httpAddResource(state->route, state->route->serverPrefix, "{controller}");
    } else {
        name = stok(sclone(value), ", \t\r\n", &next);
        while (name) {
            httpAddResource(state->route, state->route->serverPrefix, name);
            name = stok(NULL, ", \t\r\n", &next);
        }
    }
    return 0;
}


/*
    EspResourceGroup [resource ...]
 */
static int espResourceGroupDirective(MaState *state, cchar *key, cchar *value)
{
    char        *name, *next;

    if (value == 0 || *value == '\0') {
        httpAddResourceGroup(state->route, state->route->serverPrefix, "{controller}");
    } else {
        name = stok(sclone(value), ", \t\r\n", &next);
        while (name) {
            httpAddResourceGroup(state->route, state->route->serverPrefix, name);
            name = stok(NULL, ", \t\r\n", &next);
        }
    }
    return 0;
}


/*
    EspRoutePrefix /server
    Sets the route prefix to use for routes to talk to the server
 */
static int espRoutePrefixDirective(MaState *state, cchar *key, cchar *value)
{
    httpSetRouteServerPrefix(state->route, value);
    httpSetRouteVar(state->route, "SERVER_PREFIX", sjoin(state->route->prefix ? state->route->prefix: "", state->route->serverPrefix, NULL));
    return 0;
}


/*
    EspRoute 
        methods=METHODS
        name=NAME 
        prefix=PREFIX 
        source=SOURCE
        target=TARGET

    Old syntax DEPRECATED in 4.4: EspRoute name methods prefix target source
 */
static int espRouteDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    HttpRoute   *route;
    cchar       *methods, *name, *prefix, *source, *target;
    char        *option, *ovalue, *tok;

    prefix = 0;
    name = 0;
    source = 0;
    target = 0;
    methods = "GET";

    if (scontains(value, "=")) {
        for (option = maGetNextArg(sclone(value), &tok); option; option = maGetNextArg(tok, &tok)) {
            option = stok(option, "=,", &ovalue);
            ovalue = strim(ovalue, "\"'", MPR_TRIM_BOTH);
            if (smatch(option, "methods")) {
                methods = ovalue;
            } else if (smatch(option, "name")) {
                name = ovalue;
            } else if (smatch(option, "prefix")) {
                prefix = ovalue;
            } else if (smatch(option, "source")) {
                source = ovalue;
            } else if (smatch(option, "target")) {
                target = ovalue;
            } else {
                mprError("Unknown EspRoute option \"%s\"", option);
            }
        }
#if DEPRECATED || 1
    } else {
        /* 
            Deprecated in 4.4.0
         */
        if (!maTokenize(state, value, "%S %S %S %S ?S", &name, &methods, &prefix, &target, &source)) {
            return MPR_ERR_BAD_SYNTAX;
        }
#endif
    }
    if (!prefix || !target) {
        return MPR_ERR_BAD_SYNTAX;
    }
    if (target == 0 || *target == 0) {
        target = "$&";
    }
    target = stemplate(target, state->route->vars);
    if ((route = httpDefineRoute(state->route, name, methods, prefix, target, source)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    httpSetRouteHandler(route, "espHandler");
    if ((eroute = getEroute(route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (name) {
        eroute->appName = sclone(name);
    }
    return 0;
}


PUBLIC int espBindProc(HttpRoute *parent, cchar *pattern, void *proc)
{
    EspRoute    *eroute;
    HttpRoute   *route;

    if ((route = httpDefineRoute(parent, pattern, "ALL", pattern, "$&", "unused")) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    httpSetRouteHandler(route, "espHandler");

    if ((eroute = getEroute(route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    eroute->update = 0;
    espDefineAction(route, pattern, proc);
    return 0;
}


/*
    EspRouteSet kind
 */
static int espRouteSetDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    char        *kind;

    if ((eroute = getEroute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%S", &kind)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    espAddRouteSet(state->route, kind);
    return 0;
}


#if DEPRECATED || 1
/*
    EspShowErrors on|off
    Now use ShowErrors
 */
static int espShowErrorsDirective(MaState *state, cchar *key, cchar *value)
{
    bool    on;

    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    httpSetRouteShowErrors(state->route, on);
    return 0;
}
#endif


/*
    EspUpdate on|off
 */
static int espUpdateDirective(MaState *state, cchar *key, cchar *value)
{
    EspRoute    *eroute;
    bool        on;

    if ((eroute = getEroute(state->route)) == 0) {
        return MPR_ERR_MEMORY;
    }
    if (!maTokenize(state, value, "%B", &on)) {
        return MPR_ERR_BAD_SYNTAX;
    }
    eroute->update = on;
    return 0;
}


/************************************ Init ************************************/
/*
    Loadable module configuration
 */
PUBLIC int maEspHandlerInit(Http *http, MprModule *module)
{
    HttpStage   *handler;
    MaAppweb    *appweb;

    appweb = httpGetContext(http);

    if ((handler = httpCreateHandler(http, "espHandler", module)) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    http->espHandler = handler;
    handler->open = openEsp; 
    handler->close = closeEsp; 
    handler->start = startEsp; 
    if ((esp = mprAllocObj(Esp, manageEsp)) == 0) {
        return MPR_ERR_MEMORY;
    }
    handler->stageData = esp;
    MPR->espService = esp;
    esp->mutex = mprCreateLock();
    esp->local = mprCreateThreadLocal();
    if (module) {
        mprSetModuleFinalizer(module, unloadEsp);
    }
#if ME_ESP_LEGACY
    espInitHtmlOptions(esp);
#endif
    /* Thread-safe */
    if ((esp->views = mprCreateHash(-1, MPR_HASH_STATIC_VALUES)) == 0) {
        return 0;
    }
    if ((esp->actions = mprCreateHash(-1, MPR_HASH_STATIC_VALUES)) == 0) {
        return 0;
    }
    /*
        Add configuration file directives
     */
    maAddDirective(appweb, "EspApp", espAppDirective);
    maAddDirective(appweb, "<EspApp", openEspAppDirective);
    maAddDirective(appweb, "</EspApp", closeEspAppDirective);
    maAddDirective(appweb, "EspCompile", espCompileDirective);
    maAddDirective(appweb, "EspDb", espDbDirective);
    maAddDirective(appweb, "EspDir", espDirDirective);
    maAddDirective(appweb, "EspEnv", espEnvDirective);
    maAddDirective(appweb, "EspKeepSource", espKeepSourceDirective);
    maAddDirective(appweb, "EspLink", espLinkDirective);
    maAddDirective(appweb, "EspPermResource", espPermResourceDirective);
    maAddDirective(appweb, "EspResource", espResourceDirective);
    maAddDirective(appweb, "EspResourceGroup", espResourceGroupDirective);
    maAddDirective(appweb, "EspRoute", espRouteDirective);
    maAddDirective(appweb, "EspRoutePrefix", espRoutePrefixDirective);
    maAddDirective(appweb, "EspRouteSet", espRouteSetDirective);
    maAddDirective(appweb, "EspUpdate", espUpdateDirective);
#if DEPRECATED || 1
    maAddDirective(appweb, "EspShowErrors", espShowErrorsDirective);
#endif
    if ((esp->ediService = ediCreateService()) == 0) {
        return 0;
    }
#if ME_COM_MDB
    /* Memory database */
    mdbInit();
#endif
#if ME_COM_SQLITE
    /* Sqlite */
    sdbInit();
#endif
    return 0;
}


static int unloadEsp(MprModule *mp)
{
    HttpStage   *stage;

    if (esp->inUse) {
       return MPR_ERR_BUSY;
    }
    if (mprIsStopping()) {
        return 0;
    }
    if ((stage = httpLookupStage(MPR->httpService, mp->name)) != 0) {
        stage->flags |= HTTP_STAGE_UNLOADED;
    }
    return 0;
}


#else /* ME_COM_ESP */

PUBLIC int maEspHandlerInit(Http *http, MprModule *mp)
{
    mprNop(0);
    return 0;
}

#endif /* ME_COM_ESP */
/*
    @copy   default

    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */

/************************************************************************/
/*
    Start of file "src/espHtml.c"
 */
/************************************************************************/
/*
    espHtml.c -- HTML controls 

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#if ME_COM_ESP

/************************************* Local **********************************/

static cchar *getHtmlValue(HttpConn *conn, cchar *fieldName, MprHash *options);
static cchar *mapHtml(HttpConn *conn, MprHash *options);

/************************************* Code ***********************************/

PUBLIC void input(cchar *field, cchar *optionString)
{
    HttpConn    *conn;
    MprHash     *choices, *options;
    MprKey      *kp;
    EspReq      *req;
    EspRoute    *eroute;
    EdiRec      *rec;
    cchar       *rows, *cols, *etype, *value, *checked, *style, *error, *errorMsg;
    int         type, flags;

    conn = getConn();
    req = conn->data;
    eroute = req->route->eroute;
    rec = conn->record;
    if (ediGetColumnSchema(rec->edi, rec->tableName, field, &type, &flags, NULL) < 0) {
        type = -1;
    }
    options = httpGetOptions(optionString);
    style = httpGetOption(options, "class", "");
    errorMsg = rec->errors ? mprLookupKey(rec->errors, field) : 0;
    error = (errorMsg && !eroute->legacy) ? sfmt("<span class=\"field-error\">%s</span>", errorMsg) : ""; 

    switch (type) {
    case EDI_TYPE_BOOL:
        choices = httpGetOptions("{off: 0, on: 1}");
        value = getHtmlValue(conn, field, options);
        for (kp = 0; (kp = mprGetNextKey(choices, kp)) != 0; ) {
            checked = (smatch(kp->data, value)) ? " checked" : "";
            espRender(conn, "%s <input type='radio' name='%s' value='%s'%s%s class='%s'/>\r\n",
                stitle(kp->key), field, kp->data, checked, mapHtml(conn, options), style);
        }
        break;
        /* Fall through */
    case EDI_TYPE_BINARY:
    default:
        httpError(conn, 0, "espInput: unknown field type %d", type);
        /* Fall through */
    case EDI_TYPE_FLOAT:
    case EDI_TYPE_TEXT:

    case EDI_TYPE_INT:
    case EDI_TYPE_DATE:
    case EDI_TYPE_STRING:        
        if (type == EDI_TYPE_TEXT && !httpGetOption(options, "rows", 0)) {
            httpSetOption(options, "rows", "10");
        }
        etype = "text";
        value = getHtmlValue(conn, field, options);
        if (value == 0 || *value == '\0') {
            value = espGetParam(conn, field, "");
        }
        if (httpGetOption(options, "password", 0)) {
            etype = "password";
        } else if (httpGetOption(options, "hidden", 0)) {
            etype = "hidden";
        }
        if ((rows = httpGetOption(options, "rows", 0)) != 0) {
            cols = httpGetOption(options, "cols", "60");
            espRender(conn, "<textarea name='%s' type='%s' cols='%s' rows='%s'%s class='%s'>%s</textarea>", 
                field, etype, cols, rows, mapHtml(conn, options), style, value);
        } else {
            espRender(conn, "<input name='%s' type='%s' value='%s'%s class='%s'/>", field, etype, value, 
                mapHtml(conn, options), style);
        }
        if (error) {
            espRenderString(conn, error);
        }
        break;
    }
}


/*
    Render an input field with a hidden security token
    Used by esp-html-mvc to add XSRF tokens to a form
 */
PUBLIC void inputSecurityToken()
{
    HttpConn    *conn;

    conn = getConn();
    espRender(conn, "    <input name='%s' type='hidden' value='%s' />\r\n", ME_XSRF_PARAM, httpGetSecurityToken(conn, 0));
}


/**************************************** Support *************************************/ 

static cchar *getHtmlValue(HttpConn *conn, cchar *fieldName, MprHash *options)
{
    EdiRec      *record;
    EspRoute    *eroute;
    cchar       *value;

    record = conn->record;
    eroute = conn->rx->route->eroute;
    value = 0;
    if (record) {
        value = ediGetFieldValue(record, fieldName);
#if ME_ESP_LEGACY
        if (record->errors && eroute->legacy) {
            MprKey  *field;
            cchar   *msg;
            #define ESTYLE(s) "esp-" s 
            for (ITERATE_KEY_DATA(record->errors, field, msg)) {
                if (smatch(field->key, fieldName)) {
                    httpInsertOption(options, "class", ESTYLE("field-error"));
                }
            }
        }
#endif
    }
    if (value == 0) {
        value = httpGetOption(options, "value", 0);
    }
    if (!httpGetOption(options, "noescape", 0)) {
        value = mprEscapeHtml(value);
    }
    return value;
}


/*
    Map options to an attribute string.
 */
static cchar *mapHtml(HttpConn *conn, MprHash *options)
{
    MprKey      *kp;
    MprBuf      *buf;

    if (options == 0 || mprGetHashLength(options) == 0) {
        return MPR->emptyString;
    }
    buf = mprCreateBuf(-1, -1);
    for (kp = 0; (kp = mprGetNextKey(options, kp)) != 0; ) {
        if (kp->type != MPR_JSON_OBJ && kp->type != MPR_JSON_ARRAY) {
            mprPutCharToBuf(buf, ' ');
            mprPutStringToBuf(buf, kp->key);
            mprPutStringToBuf(buf, "='");
            mprPutStringToBuf(buf, kp->data);
            mprPutCharToBuf(buf, '\'');
        }
    }
    mprAddNullToBuf(buf);
    return mprGetBufStart(buf);
}

#endif /* ME_COM_ESP */
/*
    @copy   default

    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */

/************************************************************************/
/*
    Start of file "src/espTemplate.c"
 */
/************************************************************************/
/*
    espTemplate.c -- Templated web pages with embedded C code.

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#if ME_COM_ESP

/************************************ Defines *********************************/
/*
      ESP lexical analyser tokens
 */
#define ESP_TOK_ERR            -1            /* Any input error */
#define ESP_TOK_EOF             0            /* End of file */
#define ESP_TOK_CODE            1            /* <% text %> */
#define ESP_TOK_PARAM           2            /* @@param */
#define ESP_TOK_FIELD           3            /* @#field */
#define ESP_TOK_VAR             4            /* @!var */
#define ESP_TOK_HOME            5            /* @~ Home ULR */
#define ESP_TOK_SERVER          6            /* @^ Server URL  */
#define ESP_TOK_LITERAL         7            /* literal HTML */
#define ESP_TOK_EXPR            8            /* <%= expression %> */
#define ESP_TOK_CONTROL         9            /* <%@ control */

/**
    ESP page parser structure
    @defgroup EspParse EspParse
    @see
 */
typedef struct EspParse {
    int     lineNumber;                     /**< Line number for error reporting */
    char    *data;                          /**< Input data to parse */
    char    *next;                          /**< Next character in input */
    MprBuf  *token;                         /**< Input token */
} EspParse;

/************************************ Forwards ********************************/

static int getEspToken(EspParse *parse);
static cchar *getDebug(EspRoute *eroute);
static cchar *getEnvString(HttpRoute *route, cchar *key, cchar *defaultValue);
static cchar *getArExt(cchar *os);
static cchar *getShlibExt(cchar *os);
static cchar *getShobjExt(cchar *os);
static cchar *getArPath(cchar *os, cchar *arch);
static cchar *getCompilerName(cchar *os, cchar *arch);
static cchar *getCompilerPath(cchar *os, cchar *arch);
static cchar *getLibs(cchar *os);
static cchar *getMappedArch(cchar *arch);
static cchar *getObjExt(cchar *os);
static cchar *getVisualStudio();
static cchar *getWinSDK(HttpRoute *route);
static cchar *getWinVer(HttpRoute *route);
static cchar *getVxCPU(cchar *arch);
static bool matchToken(cchar **str, cchar *token);

/************************************* Code ***********************************/
/*
    Tokens:
    AR          Library archiver (ar)   
    ARLIB       Archive library extension (.a, .lib)
    ARCH        Build architecture (64)
    CC          Compiler (cc)
    DEBUG       Debug compilation options (-g, -Zi -Od)
    GCC_ARCH    ARCH mapped to gcc -arch switches (x86_64)
    INC         Include directory out/inc
    LIBPATH     Library search path
    LIBS        Libraries required to link with ESP
    OBJ         Name of compiled source (out/lib/view-MD5.o)
    MOD         Output module (view_MD5)
    SHLIB       Host Shared library (.lib, .so)
    SHOBJ       Host Shared Object (.dll, .so)
    SRC         Source code for view or controller (already templated)
    TMP         Temp directory
    VS          Visual Studio directory
    WINSDK      Windows SDK directory
 */
PUBLIC char *espExpandCommand(HttpRoute *route, cchar *command, cchar *source, cchar *module)
{
    MprBuf      *buf;
    MaAppweb    *appweb;
    EspRoute    *eroute;
    cchar       *cp, *outputModule, *os, *arch, *profile;
    char        *tmp;
    
    if (command == 0) {
        return 0;
    }
    appweb = MPR->appwebService;
    eroute = route->eroute;
    outputModule = mprTrimPathExt(module);
    maParsePlatform(appweb->platform, &os, &arch, &profile);
    buf = mprCreateBuf(-1, -1);

    for (cp = command; *cp; ) {
        if (*cp == '$') {
            if (matchToken(&cp, "${ARCH}")) {
                /* Target architecture (x86|mips|arm|x64) */
                mprPutStringToBuf(buf, arch);

            } else if (matchToken(&cp, "${ARLIB}")) {
                /* .a, .lib */
                mprPutStringToBuf(buf, getArExt(os));

            } else if (matchToken(&cp, "${GCC_ARCH}")) {
                /* Target architecture mapped to GCC mtune|mcpu values */
                mprPutStringToBuf(buf, getMappedArch(arch));

            } else if (matchToken(&cp, "${APPINC}")) {
                /* Application src include directory */
                mprPutStringToBuf(buf, eroute->srcDir ? eroute->srcDir : ".");

            } else if (matchToken(&cp, "${INC}")) {
                /* Include directory for the configuration */
                mprPutStringToBuf(buf, mprJoinPath(appweb->platformDir, "inc")); 

            } else if (matchToken(&cp, "${LIBPATH}")) {
                /* Library directory for Appweb libraries for the target */
                mprPutStringToBuf(buf, mprJoinPath(appweb->platformDir, "bin")); 

            } else if (matchToken(&cp, "${LIBS}")) {
                /* Required libraries to link. These may have nested ${TOKENS} */
                mprPutStringToBuf(buf, espExpandCommand(route, getLibs(os), source, module));

            } else if (matchToken(&cp, "${MOD}")) {
                /* Output module path in the cache without extension */
                mprPutStringToBuf(buf, outputModule);

            } else if (matchToken(&cp, "${OBJ}")) {
                /* Output object with extension (.o) in the cache directory */
                mprPutStringToBuf(buf, mprJoinPathExt(outputModule, getObjExt(os)));

            } else if (matchToken(&cp, "${OS}")) {
                /* Target architecture (freebsd|linux|macosx|windows|vxworks) */
                mprPutStringToBuf(buf, os);

            } else if (matchToken(&cp, "${SHLIB}")) {
                /* .dll, .so, .dylib */
                mprPutStringToBuf(buf, getShlibExt(os));

            } else if (matchToken(&cp, "${SHOBJ}")) {
                /* .dll, .so, .dylib */
                mprPutStringToBuf(buf, getShobjExt(os));

            } else if (matchToken(&cp, "${SRC}")) {
                /* View (already parsed into C code) or controller source */
                mprPutStringToBuf(buf, source);

            } else if (matchToken(&cp, "${TMP}")) {
                if ((tmp = getenv("TMPDIR")) == 0) {
                    if ((tmp = getenv("TMP")) == 0) {
                        tmp = getenv("TEMP");
                    }
                }
                mprPutStringToBuf(buf, tmp ? tmp : ".");

            } else if (matchToken(&cp, "${VS}")) {
                mprPutStringToBuf(buf, getVisualStudio());

            } else if (matchToken(&cp, "${VXCPU}")) {
                mprPutStringToBuf(buf, getVxCPU(arch));

            } else if (matchToken(&cp, "${WINSDK}")) {
                mprPutStringToBuf(buf, getWinSDK(route));

            } else if (matchToken(&cp, "${WINVER}")) {
                mprPutStringToBuf(buf, getWinVer(route));

            /*
                These vars can be also be configured from environment variables.
                NOTE: the default esp.conf includes "esp->vxworks.conf" which has EspEnv definitions for the 
                configured VxWorks toolchain.
             */
            } else if (matchToken(&cp, "${AR}")) {
                mprPutStringToBuf(buf, getEnvString(route, "AR", getArPath(os, arch)));

            } else if (matchToken(&cp, "${CC}")) {
                mprPutStringToBuf(buf, getEnvString(route, "CC", getCompilerPath(os, arch)));

            } else if (matchToken(&cp, "${CFLAGS}")) {
                mprPutStringToBuf(buf, getEnvString(route, "CFLAGS", ""));

            } else if (matchToken(&cp, "${DEBUG}")) {
                mprPutStringToBuf(buf, getEnvString(route, "DEBUG", getDebug(eroute)));

            } else if (matchToken(&cp, "${LDFLAGS}")) {
                mprPutStringToBuf(buf, getEnvString(route, "LDFLAGS", ""));

            } else if (matchToken(&cp, "${LIB}")) {
                mprPutStringToBuf(buf, getEnvString(route, "LIB", ""));

            } else if (matchToken(&cp, "${LINK}")) {
                mprPutStringToBuf(buf, getEnvString(route, "LINK", ""));

            } else if (matchToken(&cp, "${WIND_BASE}")) {
                mprPutStringToBuf(buf, getEnvString(route, "WIND_BASE", WIND_BASE));

            } else if (matchToken(&cp, "${WIND_HOME}")) {
                mprPutStringToBuf(buf, getEnvString(route, "WIND_HOME", WIND_HOME));

            } else if (matchToken(&cp, "${WIND_HOST_TYPE}")) {
                mprPutStringToBuf(buf, getEnvString(route, "WIND_HOST_TYPE", WIND_HOST_TYPE));

            } else if (matchToken(&cp, "${WIND_PLATFORM}")) {
                mprPutStringToBuf(buf, getEnvString(route, "WIND_PLATFORM", WIND_PLATFORM));

            } else if (matchToken(&cp, "${WIND_GNU_PATH}")) {
                mprPutStringToBuf(buf, getEnvString(route, "WIND_GNU_PATH", WIND_GNU_PATH));

            } else if (matchToken(&cp, "${WIND_CCNAME}")) {
                mprPutStringToBuf(buf, getEnvString(route, "WIND_CCNAME", getCompilerName(os, arch)));

            } else {
                mprPutCharToBuf(buf, *cp++);
            }
        } else {
            mprPutCharToBuf(buf, *cp++);
        }
    }
    mprAddNullToBuf(buf);
    return sclone(mprGetBufStart(buf));
}


static int runCommand(HttpRoute *route, MprDispatcher *dispatcher, cchar *command, cchar *csource, cchar *module, char **errMsg)
{
    MprCmd      *cmd;
    MprKey      *var;
    MprList     *elist;
    EspRoute    *eroute;
    cchar       **env, *commandLine;
    char        *err, *out;
    int         rc;

    *errMsg = 0;
    eroute = route->eroute;
    cmd = mprCreateCmd(dispatcher);
    if ((commandLine = espExpandCommand(route, command, csource, module)) == 0) {
        *errMsg = sfmt("Missing EspCompile directive for %s", csource);
        return MPR_ERR_CANT_READ;
    }
    mprTrace(4, "ESP command: %s\n", commandLine);
    if (eroute->env) {
        elist = mprCreateList(0, MPR_LIST_STABLE);
        for (ITERATE_KEYS(eroute->env, var)) {
            mprAddItem(elist, sfmt("%s=%s", var->key, (char*) var->data));
        }
        mprAddNullItem(elist);
        env = (cchar**) &elist->items[0];
    } else {
        env = 0;
    }
    if (eroute->searchPath) {
        mprSetCmdSearchPath(cmd, eroute->searchPath);
    }
    /*
        WARNING: GC will run here
     */
    mprHold((void*) commandLine);
    rc = mprRunCmd(cmd, commandLine, env, NULL, &out, &err, -1, 0);
    mprRelease((void*) commandLine);

    if (rc != 0) {
        if (err == 0 || *err == '\0') {
            /* Windows puts errors to stdout Ugh! */
            err = out;
        }
        mprError("ESP: Cannot run command: %s, error %s", commandLine, err);
        if (route->flags & HTTP_ROUTE_SHOW_ERRORS) {
            *errMsg = sfmt("Cannot run command: %s, error %s", commandLine, err);
        } else {
            *errMsg = "Cannot compile view";
        }
        return MPR_ERR_CANT_COMPLETE;
    }
    return 0;
}


/*
    Compile a view or controller

    cacheName   MD5 cache name (not a full path)
    source      ESP source file name
    module      Module file name

    WARNING: this routine blocks and runs GC. All parameters must be retained.
 */
PUBLIC bool espCompile(HttpRoute *route, MprDispatcher *dispatcher, cchar *source, cchar *module, cchar *cacheName, int isView, char **errMsg)
{
    MprFile     *fp;
    EspRoute    *eroute;
    cchar       *csource;
    char        *layout, *script, *page, *err;
    ssize       len;

    eroute = route->eroute;
    layout = 0;
    *errMsg = 0;

    mprLog(2, "esp: compile %s", source);
    if (isView) {
        if ((page = mprReadPathContents(source, &len)) == 0) {
            *errMsg = sfmt("Cannot read %s", source);
            return 0;
        }
        /*
            Use layouts iff there is a source defined on the route. Only MVC/controllers based apps do this.
         */
        if (eroute->layoutsDir) {
            layout = mprJoinPath(eroute->layoutsDir, "default.esp");
        }
        if ((script = espBuildScript(route, page, source, cacheName, layout, NULL, &err)) == 0) {
            *errMsg = sfmt("Cannot build: %s, error: %s", source, err);
            return 0;
        }
        csource = mprJoinPathExt(mprTrimPathExt(module), ".c");
        mprMakeDir(mprGetPathDir(csource), 0775, -1, -1, 1);
        if ((fp = mprOpenFile(csource, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, 0664)) == 0) {
            *errMsg = sfmt("Cannot open compiled script file %s", csource);
            return 0;
        }
        len = slen(script);
        if (mprWriteFile(fp, script, len) != len) {
            *errMsg = sfmt("Cannot write compiled script file %s", csource);
            mprCloseFile(fp);
            return 0;
        }
        mprCloseFile(fp);
    } else {
        csource = source;
    }
    mprMakeDir(mprGetPathDir(module), 0775, -1, -1, 1);

#if ME_WIN_LIKE
    {
        /*
            Force a clean windows compile by removing the obj, pdb and ilk files
         */
        cchar   *path;
        path = mprReplacePathExt(module, "obj");
        if (mprPathExists(path, F_OK)) {
            mprDeletePath(path);
        }
        path = mprReplacePathExt(module, "pdb");
        if (mprPathExists(path, F_OK)) {
            mprDeletePath(path);
        }
        path = mprReplacePathExt(module, "ilk");
        if (mprPathExists(path, F_OK)) {
            mprDeletePath(path);
        }
    }
#endif
    /* WARNING: GC yield here */
    if (runCommand(route, dispatcher, eroute->compile, csource, module, errMsg) != 0) {
        return 0;
    }
    if (eroute->link) {
        /* WARNING: GC yield here */
        if (runCommand(route, dispatcher, eroute->link, csource, module, errMsg) != 0) {
            return 0;
        }
#if !(ME_DEBUG && MACOSX)
        /*
            MAC needs the object for debug information
         */
        mprDeletePath(mprJoinPathExt(mprTrimPathExt(module), &ME_OBJ[1]));
#endif
    }
#if ME_WIN_LIKE
    {
        /*
            Windows leaves intermediate object in the current directory
         */
        cchar   *path;
        path = mprReplacePathExt(mprGetPathBase(csource), "obj");
        if (mprPathExists(path, F_OK)) {
            mprDeletePath(path);
        }
    }
#endif
    if (!eroute->keepSource && isView) {
        mprDeletePath(csource);
    }
    return 1;
}


static char *fixMultiStrings(cchar *str)
{
    cchar   *cp;
    char    *buf, *bp;
    ssize   len;
    int     count, bquote, quoted;

    for (count = 0, cp = str; *cp; cp++) {
        if (*cp == '\n' || *cp == '"') {
            count++;
        }
    }
    len = slen(str);
    if ((buf = mprAlloc(len + (count * 3) + 1)) == 0) {
        return 0;
    }
    bquote = quoted = 0;
    for (cp = str, bp = buf; *cp; cp++) {
        if (*cp == '`') {
            *bp++ = '"';
            quoted = !quoted;
        } else if (quoted) {
            if (*cp == '\n') {
                *bp++ = '\\';
            } else if (*cp == '"') {
                *bp++ = '\\';
            } else if (*cp == '\\' && cp[1] != '\\') {
                bquote++;
            }
            *bp++ = *cp;
        } else {
            *bp++ = *cp;
        }
    }
    *bp = '\0';
    return buf;
}


static char *joinLine(cchar *str, ssize *lenp)
{
    cchar   *cp;
    char    *buf, *bp;
    ssize   len;
    int     count, bquote;

    for (count = 0, cp = str; *cp; cp++) {
        if (*cp == '\n' || *cp == '\r') {
            count++;
        }
    }
    len = slen(str);
    if ((buf = mprAlloc(len + (count * 3) + 1)) == 0) {
        return 0;
    }
    bquote = 0;
    for (cp = str, bp = buf; *cp; cp++) {
        if (*cp == '\n') {
            *bp++ = '\\';
            *bp++ = 'n';
            *bp++ = '\\';
        } else if (*cp == '\r') {
            *bp++ = '\\';
            *bp++ = 'r';
            continue;
        } else if (*cp == '\\') {
            if (cp[1]) {
                *bp++ = *cp++;
                bquote++;
            }
        }
        *bp++ = *cp;
    }
    *bp = '\0';
    *lenp = len - bquote;
    return buf;
}


/*
    Convert an ESP web page into C code
    Directives:
        <%@ include "file"  Include an esp file
        <%@ layout "file"   Specify a layout page to use. Use layout "" to disable layout management
        <%@ content         Mark the location to substitute content in a layout page

        <%                  Begin esp section containing C code
        <%^ global          Put esp code at the global level
        <%^ start           Put esp code at the start of the function
        <%^ end             Put esp code at the end of the function

        <%=                 Begin esp section containing an expression to evaluate and substitute
        <%= [%fmt]          Begin a formatted expression to evaluate and substitute. Format is normal printf format.
                            Use %S for safe HTML escaped output.
        %>                  End esp section
        -%>                 End esp section and trim trailing newline

        @@var               Substitue the value of a parameter. 
        @!var               Substitue the value of a variable. Var can also be simple expressions (without spaces)
        @#field             Lookup the current record for the value of the field.
        @~                  Home URL for the application
        @|                  Top level server side URL

 */
PUBLIC char *espBuildScript(HttpRoute *route, cchar *page, cchar *path, cchar *cacheName, cchar *layout, 
        EspState *state, char **err)
{
    EspRoute    *eroute;
    EspState    top;
    EspParse    parse;
    MprBuf      *body;
    char        *control, *incText, *where, *layoutCode, *bodyCode;
    char        *rest, *include, *line, *fmt, *layoutPage, *incCode, *token;
    ssize       len;
    int         tid;

    assert(page);

    *err = 0;
    eroute = route->eroute;
    if (!state) {
        assert(cacheName);
        state = &top;
        memset(state, 0, sizeof(EspParse));
        state->global = mprCreateBuf(0, 0);
        state->start = mprCreateBuf(0, 0);
        state->end = mprCreateBuf(0, 0);
    }
    body = mprCreateBuf(0, 0);
    parse.data = (char*) page;
    parse.next = parse.data;
    parse.lineNumber = 0;
    parse.token = mprCreateBuf(0, 0);
    tid = getEspToken(&parse);

    while (tid != ESP_TOK_EOF) {
        token = mprGetBufStart(parse.token);
#if KEEP
        if (state->lineNumber != lastLine) {
            mprPutToBuf(script, "\n# %d \"%s\"\n", state->lineNumber, path);
        }
#endif
        switch (tid) {
        case ESP_TOK_CODE:
            if (*token == '^') {
                for (token++; *token && isspace((uchar) *token); token++) ;
                where = stok(token, " \t\r\n", &rest);
                if (rest == 0) {
                    ;
                } else if (scmp(where, "global") == 0) {
                    mprPutStringToBuf(state->global, rest);

                } else if (scmp(where, "start") == 0) {
                    mprPutToBuf(state->start, "%s  ", rest);

                } else if (scmp(where, "end") == 0) {
                    mprPutToBuf(state->end, "%s  ", rest);
                }
            } else {
                mprPutStringToBuf(body, fixMultiStrings(token));
            }
            break;

        case ESP_TOK_CONTROL:
            control = stok(token, " \t\r\n", &token);
            if (scmp(control, "content") == 0) {
                mprPutStringToBuf(body, ESP_CONTENT_MARKER);

            } else if (scmp(control, "include") == 0) {
                if (token == 0) {
                    token = "";
                }
                token = strim(token, " \t\r\n\"", MPR_TRIM_BOTH);
                token = mprNormalizePath(token);
                if (token[0] == '/') {
                    include = sclone(token);
                } else {
                    include = mprJoinPath(mprGetPathDir(path), token);
                }
                if ((incText = mprReadPathContents(include, &len)) == 0) {
                    *err = sfmt("Cannot read include file: %s", include);
                    return 0;
                }
                /* Recurse and process the include script */
                if ((incCode = espBuildScript(route, incText, include, NULL, NULL, state, err)) == 0) {
                    return 0;
                }
                mprPutStringToBuf(body, incCode);

            } else if (scmp(control, "layout") == 0) {
                token = strim(token, " \t\r\n\"", MPR_TRIM_BOTH);
                if (*token == '\0') {
                    layout = 0;
                } else {
                    token = mprNormalizePath(token);
                    if (token[0] == '/') {
                        layout = sclone(token);
                    } else {
                        layout = mprJoinPath(eroute->layoutsDir ? eroute->layoutsDir : mprGetPathDir(path), token);
                    }
                    if (!mprPathExists(layout, F_OK)) {
                        *err = sfmt("Cannot access layout page %s", layout);
                        return 0;
                    }
                }

            } else {
                *err = sfmt("Unknown control %s at line %d", control, state->lineNumber);
                return 0;
            }
            break;

        case ESP_TOK_ERR:
            return 0;

        case ESP_TOK_EXPR:
            /* <%= expr %> */
            if (*token == '%') {
                fmt = stok(token, ": \t\r\n", &token);
                if (token == 0) { 
                    token = "";
                }
                /* Default without format is safe. If users want a format and safe, use %S or renderSafe() */
                token = strim(token, " \t\r\n;", MPR_TRIM_BOTH);
                mprPutToBuf(body, "  espRender(conn, \"%s\", %s);\n", fmt, token);
            } else {
                token = strim(token, " \t\r\n;", MPR_TRIM_BOTH);
                mprPutToBuf(body, "  espRenderSafeString(conn, %s);\n", token);
            }
            break;

        case ESP_TOK_FIELD:
            /* @#field -- field in the current record */
            token = strim(token, " \t\r\n;", MPR_TRIM_BOTH);
            mprPutToBuf(body, "  espRenderSafeString(conn, getField(getRec(), \"%s\"));\n", token);
            break;

        case ESP_TOK_PARAM:
            /* @@var -- variable in (param || session) - Safe render */
            token = strim(token, " \t\r\n;", MPR_TRIM_BOTH);
            mprPutToBuf(body, "  espRenderVar(conn, \"%s\");\n", token);
            break;

        case ESP_TOK_VAR:
            /* @!var -- string variable */
            token = strim(token, " \t\r\n;", MPR_TRIM_BOTH);
            mprPutToBuf(body, "  espRenderString(conn, %s);\n", token);
            break;

        case ESP_TOK_HOME:
            /* @~ Home URL */
            mprPutToBuf(body, "  espRenderString(conn, conn->rx->route->prefix);");
            break;

        case ESP_TOK_SERVER:
            /* @^ Server URL */
            mprPutToBuf(body, "  espRenderString(conn, sjoin(conn->rx->route->prefix ? conn->rx->route->prefix : \"\", conn->rx->route->serverPrefix, NULL));");
            break;

        case ESP_TOK_LITERAL:
            line = joinLine(token, &len);
            mprPutToBuf(body, "  espRenderBlock(conn, \"%s\", %zd);\n", line, len);
            break;

        default:
            return 0;
        }
        tid = getEspToken(&parse);
    }
    mprAddNullToBuf(body);

    if (layout && mprPathExists(layout, R_OK)) {
        if ((layoutPage = mprReadPathContents(layout, &len)) == 0) {
            *err = sfmt("Cannot read layout page: %s", layout);
            return 0;
        }
        if ((layoutCode = espBuildScript(route, layoutPage, layout, NULL, NULL, state, err)) == 0) {
            return 0;
        }
#if ME_DEBUG
        if (!scontains(layoutCode, ESP_CONTENT_MARKER)) {
            *err = sfmt("Layout page is missing content marker: %s", layout);
            return 0;
        }
#endif
        bodyCode = sreplace(layoutCode, ESP_CONTENT_MARKER, mprGetBufStart(body));
    } else {
        bodyCode = mprGetBufStart(body);
    }
    if (state == &top) {
        path = mprGetRelPath(path, route->documents);
        if (mprGetBufLength(state->start) > 0) {
            mprPutCharToBuf(state->start, '\n');
        }
        if (mprGetBufLength(state->end) > 0) {
            mprPutCharToBuf(state->end, '\n');
        }
        mprAddNullToBuf(state->global);
        mprAddNullToBuf(state->start);
        mprAddNullToBuf(state->end);
        bodyCode = sfmt(\
            "/*\n   Generated from %s\n */\n"\
            "#include \"esp.h\"\n"\
            "%s\n"\
            "static void %s(HttpConn *conn) {\n"\
            "%s%s%s"\
            "}\n\n"\
            "%s int esp_%s(HttpRoute *route, MprModule *module) {\n"\
            "   espDefineView(route, \"%s\", %s);\n"\
            "   return 0;\n"\
            "}\n",
            path, mprGetBufStart(state->global), cacheName, mprGetBufStart(state->start), bodyCode, mprGetBufStart(state->end),
            ESP_EXPORT_STRING, cacheName, mprGetPortablePath(path), cacheName);
        mprTrace(6, "Create ESP script: \n%s\n", bodyCode);
    }
    return bodyCode;
}


static bool addChar(EspParse *parse, int c)
{
    if (mprPutCharToBuf(parse->token, c) < 0) {
        return 0;
    }
    mprAddNullToBuf(parse->token);
    return 1;
}


static char *eatSpace(EspParse *parse, char *next)
{
    for (; *next && isspace((uchar) *next); next++) {
        if (*next == '\n') {
            parse->lineNumber++;
        }
    }
    return next;
}


static char *eatNewLine(EspParse *parse, char *next)
{
    for (; *next && isspace((uchar) *next); next++) {
        if (*next == '\n') {
            parse->lineNumber++;
            next++;
            break;
        }
    }
    return next;
}


/*
    Get the next ESP input token. input points to the next input token.
    parse->token will hold the parsed token. The function returns the token id.
 */
static int getEspToken(EspParse *parse)
{
    char    *start, *end, *next;
    int     tid, done, c, t;

    start = next = parse->next;
    end = &start[slen(start)];
    mprFlushBuf(parse->token);
    tid = ESP_TOK_LITERAL;

    for (done = 0; !done && next < end; next++) {
        c = *next;
        switch (c) {
        case '<':
            if (next[1] == '%' && ((next == start) || next[-1] != '\\')) {
                next += 2;
                if (mprGetBufLength(parse->token) > 0) {
                    next -= 3;
                } else {
                    next = eatSpace(parse, next);
                    if (*next == '=') {
                        /*
                            <%= directive
                         */
                        tid = ESP_TOK_EXPR;
                        next = eatSpace(parse, ++next);
                        while (next < end && !(*next == '%' && next[1] == '>' && next[-1] != '\\')) {
                            if (*next == '\n') parse->lineNumber++;
                            if (!addChar(parse, *next++)) {
                                return ESP_TOK_ERR;
                            }
                        }

                    } else if (*next == '@') {
                        tid = ESP_TOK_CONTROL;
                        next = eatSpace(parse, ++next);
                        while (next < end && !(*next == '%' && next[1] == '>' && next[-1] != '\\')) {
                            if (*next == '\n') parse->lineNumber++;
                            if (!addChar(parse, *next++)) {
                                return ESP_TOK_ERR;
                            }
                        }
                        
                    } else {
                        tid = ESP_TOK_CODE;
                        while (next < end && !(*next == '%' && next[1] == '>' && next[-1] != '\\')) {
                            if (*next == '\n') parse->lineNumber++;
                            if (!addChar(parse, *next++)) {
                                return ESP_TOK_ERR;
                            }
                        }
                    }
                    if (*next && next > start && next[-1] == '-') {
                        /* Remove "-" */
                        mprAdjustBufEnd(parse->token, -1);
                        mprAddNullToBuf(parse->token);
                        next = eatNewLine(parse, next + 2) - 1;
                    } else {
                        next++;
                    }
                }
                done++;
            } else {
                if (!addChar(parse, c)) {
                    return ESP_TOK_ERR;
                }                
            }
            break;

        case '@':
            if ((next == start) || next[-1] != '\\') {
                t = next[1];
                if (t == '~') {
                    next += 2;
                    if (mprGetBufLength(parse->token) > 0) {
                        next -= 3;
                    } else {
                        tid = ESP_TOK_HOME;
                        if (!addChar(parse, c)) {
                            return ESP_TOK_ERR;
                        }
                        next--;
                    }
                    done++;

                } else if (t == ME_SERVER_PREFIX_CHAR) {
                    next += 2;
                    if (mprGetBufLength(parse->token) > 0) {
                        next -= 3;
                    } else {
                        tid = ESP_TOK_SERVER;
                        if (!addChar(parse, c)) {
                            return ESP_TOK_ERR;
                        }
                        next--;
                    }
                    done++;

                } else if (t == '@' || t == '#' || t == '!') {
                    next += 2;
                    if (mprGetBufLength(parse->token) > 0) {
                        next -= 3;
                    } else {
                        if (t == '!') {
                           tid = ESP_TOK_VAR;
                        } else if (t == '#') {
                            tid = ESP_TOK_FIELD;
                        } else {
                            tid = ESP_TOK_PARAM;
                        }
                        next = eatSpace(parse, next);
                        while (isalnum((uchar) *next) || *next == '_') {
                            if (*next == '\n') parse->lineNumber++;
                            if (!addChar(parse, *next++)) {
                                return ESP_TOK_ERR;
                            }
                        }
                        next--;
                    }
                    done++;
                } else {
                    if (!addChar(parse, c)) {
                        return ESP_TOK_ERR;
                    }
                    done++;
                }
            }
            break;

        case '\n':
            parse->lineNumber++;
            /* Fall through */

        case '\r':
        default:
            if (c == '\"' || c == '\\') {
                if (!addChar(parse, '\\')) {
                    return ESP_TOK_ERR;
                }
            }
            if (!addChar(parse, c)) {
                return ESP_TOK_ERR;
            }
            break;
        }
    }
    if (mprGetBufLength(parse->token) == 0) {
        tid = ESP_TOK_EOF;
    }
    parse->next = next;
    return tid;
}


static cchar *getEnvString(HttpRoute *route, cchar *key, cchar *defaultValue)
{
    EspRoute    *eroute;
    cchar       *value;

    eroute = route->eroute;
    if (!eroute || !eroute->env || (value = mprLookupKey(eroute->env, key)) == 0) {
        if ((value = getenv(key)) == 0) {
            if (defaultValue) {
                value = defaultValue;
            } else {
                value = sfmt("${%s}", key);
            }
        }
    }
    return value;
}


static cchar *getShobjExt(cchar *os)
{
    if (smatch(os, "macosx")) {
        return ".dylib";
    } else if (smatch(os, "windows")) {
        return ".dll";
    } else if (smatch(os, "vxworks")) {
        return ".out";
    } else {
        return ".so";
    }
}


static cchar *getShlibExt(cchar *os)
{
    if (smatch(os, "macosx")) {
        return ".dylib";
    } else if (smatch(os, "windows")) {
        return ".lib";
    } else if (smatch(os, "vxworks")) {
        return ".a";
    } else {
        return ".so";
    }
}


static cchar *getObjExt(cchar *os)
{
    if (smatch(os, "windows")) {
        return ".obj";
    }
    return ".o";
}


static cchar *getArExt(cchar *os)
{
    if (smatch(os, "windows")) {
        return ".lib";
    }
    return ".a";
}


static cchar *getCompilerName(cchar *os, cchar *arch)
{
    cchar       *name;

    name = "gcc";
    if (smatch(os, "vxworks")) {
        if (smatch(arch, "x86") || smatch(arch, "i586") || smatch(arch, "i686") || smatch(arch, "pentium")) {
            name = "ccpentium";
        } else if (scontains(arch, "86")) {
            name = "cc386";
        } else if (scontains(arch, "ppc")) {
            name = "ccppc";
        } else if (scontains(arch, "xscale") || scontains(arch, "arm")) {
            name = "ccarm";
        } else if (scontains(arch, "68")) {
            name = "cc68k";
        } else if (scontains(arch, "sh")) {
            name = "ccsh";
        } else if (scontains(arch, "mips")) {
            name = "ccmips";
        }
    } else if (smatch(os, "macosx")) {
        name = "clang";
    }
    return name;
}


static cchar *getVxCPU(cchar *arch)
{
    char   *cpu, *family;

    family = stok(sclone(arch), ":", &cpu);
    if (!cpu || *cpu == '\0') {
        if (smatch(family, "i386")) {
            cpu = "I80386";
        } else if (smatch(family, "i486")) {
            cpu = "I80486";
        } else if (smatch(family, "x86") | sends(family, "86")) {
            cpu = "PENTIUM";
        } else if (scaselessmatch(family, "mips")) {
            cpu = "MIPS32";
        } else if (scaselessmatch(family, "arm")) {
            cpu = "ARM7TDMI";
        } else if (scaselessmatch(family, "ppc")) {
            cpu = "PPC";
        } else {
            cpu = (char*) arch;
        }
    }
    return supper(cpu);
}


static cchar *getDebug(EspRoute *eroute)
{
    MaAppweb    *appweb;
    Esp         *esp;
    int         debug;

    appweb = MPR->appwebService;
    esp = MPR->espService;
    if (esp->compileMode == ESP_COMPILE_DEBUG) {
        debug = 1;
    } else if (esp->compileMode == ESP_COMPILE_RELEASE) {
        debug = 0;
    } else if (eroute->compileMode == ESP_COMPILE_DEBUG) {
        debug = 1;
    } else if (eroute->compileMode == ESP_COMPILE_RELEASE) {
        debug = 0;
    } else {
        debug = sends(appweb->platform, "-debug") || sends(appweb->platform, "-xcode") || 
            sends(appweb->platform, "-mine") || sends(appweb->platform, "-vsdebug");
    }
    if (scontains(appweb->platform, "windows-")) {
        return (debug) ? "-DME_DEBUG -Zi -Od" : "-Os";
    }
    return (debug) ? "-DME_DEBUG -g" : "-O2";
}


static cchar *getLibs(cchar *os)
{
    cchar       *libs;

    if (smatch(os, "windows")) {
        libs = "\"${LIBPATH}\\libmod_esp${SHLIB}\" \"${LIBPATH}\\libappweb.lib\" \"${LIBPATH}\\libhttp.lib\" \"${LIBPATH}\\libmpr.lib\"";
    } else {
#if LINUX
        /* 
            Fedora interprets $ORIGN relative to the shared library and not the application executable
            So loading compiled apps fails to locate libmod_esp.so. 
            Since building a shared library, can omit libs and resolve at load time.
         */
        libs = "";
#else
        libs = "-lmod_esp -lappweb -lpcre -lhttp -lmpr -lpthread -lm";
#endif
    }
    return libs;
}


static bool matchToken(cchar **str, cchar *token)
{
    ssize   len;

    len = slen(token);
    if (sncmp(*str, token, len) == 0) {
        *str += len;
        return 1;
    }
    return 0;
}


static cchar *getMappedArch(cchar *arch)
{
    if (smatch(arch, "x64")) {
        arch = "x86_64";
    } else if (smatch(arch, "x86")) {
        arch = "i686";
    }
    return arch;
}


#if WINDOWS
//  TODO - move to mpr
static int reverseSortVersions(char **s1, char **s2)
{
    return -scmp(*s1, *s2);
}
#endif


static cchar *getWinSDK(HttpRoute *route)
{
#if WINDOWS
    EspRoute *eroute;

    /*
        MS has made a huge mess of where and how the windows SDKs are installed. The registry key at 
        HKLM/Software/Microsoft/Microsoft SDKs/Windows/CurrentInstallFolder can't be trusted and often
        points to the old 7.X SDKs even when 8.X is installed and active. MS have also moved the 8.X
        SDK to Windows Kits, while still using the old folder for some bits. So the old-reliable
        CurrentInstallFolder registry key is now unusable. So we must scan for explicit SDK versions 
        listed above. Ugh!
     */
    cchar   *path, *key, *version;
    MprList *versions;
    int     i;

    eroute = route->eroute;
    if (eroute->winsdk) {
        return eroute->winsdk;
    }
    /* 
        General strategy is to find an "include" directory in the highest version Windows SDK.
        First search the registry key: Windows Kits/InstalledRoots/KitsRoot*
     */
    key = sfmt("HKLM\\SOFTWARE%s\\Microsoft\\Windows Kits\\Installed Roots", (ME_64) ? "\\Wow6432Node" : "");
    versions = mprListRegistry(key);
    mprSortList(versions, (MprSortProc) reverseSortVersions, 0);
    path = 0;
    for (ITERATE_ITEMS(versions, version, i)) {
        if (scontains(version, "KitsRoot")) {
            path = mprReadRegistry(key, version);
            if (mprPathExists(mprJoinPath(path, "Include"), X_OK)) {
                break;
            }
            path = 0;
        }
    }
    if (!path) {
        /* 
            Next search the registry keys at Windows SDKs/Windows/ * /InstallationFolder
         */
        key = sfmt("HKLM\\SOFTWARE%s\\Microsoft\\Microsoft SDKs\\Windows", (ME_64) ? "\\Wow6432Node" : "");
        versions = mprListRegistry(key);
        mprSortList(versions, (MprSortProc) reverseSortVersions, 0);
        for (ITERATE_ITEMS(versions, version, i)) {
            if ((path = mprReadRegistry(sfmt("%s\\%s", key, version), "InstallationFolder")) != 0) {
                if (mprPathExists(mprJoinPath(path, "Include"), X_OK)) {
                    break;
                }
                path = 0;
            }
        }
    }
    if (!path) {
        /* Last chance: Old Windows SDK 7 registry location */
        path = mprReadRegistry("HKLM\\SOFTWARE\\Microsoft\\Microsoft SDKs\\Windows", "CurrentInstallFolder");
    }
    if (!path) {
        path = "${WINSDK}";
    }
    mprLog(4, "Using Windows SDK at %s", path);
    eroute->winsdk = strim(path, "\\", MPR_TRIM_END);
    return eroute->winsdk;
#else
    return "";
#endif
}


static cchar *getWinVer(HttpRoute *route)
{
    MprList     *versions;
    cchar       *winver, *winsdk;

    winsdk = getWinSDK(route);
    versions = mprGlobPathFiles(mprJoinPath(winsdk, "Lib"), "*", MPR_PATH_RELATIVE);
    mprSortList(versions, 0, 0);
    if ((winver = mprGetLastItem(versions)) == 0) {
        winver = sclone("win8");
    }
    return winver;
}


static cchar *getVisualStudio()
{
#if WINDOWS
    cchar   *path;
    int     v;
    /* VS 2013 == 12.0 */
    for (v = 16; v >= 8; v--) {
        if ((path = mprReadRegistry(ESP_VSKEY, sfmt("%d.0", v))) != 0) {
            path = strim(path, "\\", MPR_TRIM_END);
            break;
        }
    }
    if (!path) {
        path = "${VS}";
    }
    return path;
#else
    return "";
#endif
}


static cchar *getArPath(cchar *os, cchar *arch)
{
#if WINDOWS
    /* 
        Get the real system architecture (32 or 64 bit)
     */
    MaAppweb *appweb = MPR->appwebService;
    cchar *path = getVisualStudio();
    if (scontains(appweb->platform, "-x64-")) {
        int is64BitSystem = smatch(getenv("PROCESSOR_ARCHITECTURE"), "AMD64") || getenv("PROCESSOR_ARCHITEW6432");
        if (is64BitSystem) {
            path = mprJoinPath(path, "VC/bin/amd64/lib.exe");
        } else {
            /* Cross building on a 32-bit system */
            path = mprJoinPath(path, "VC/bin/x86_amd64/lib.exe");
        }
    } else {
        path = mprJoinPath(path, "VC/bin/lib.exe");
    }
    return path;
#else
    return "ar";
#endif
}


static cchar *getCompilerPath(cchar *os, cchar *arch)
{
#if WINDOWS
    /* 
        Get the real system architecture (32 or 64 bit)
     */
    MaAppweb *appweb = MPR->appwebService;
    cchar *path = getVisualStudio();
    if (scontains(appweb->platform, "-x64-")) {
        int is64BitSystem = smatch(getenv("PROCESSOR_ARCHITECTURE"), "AMD64") || getenv("PROCESSOR_ARCHITEW6432");
        if (is64BitSystem) {
            path = mprJoinPath(path, "VC/bin/amd64/cl.exe");
        } else {
            /* Cross building on a 32-bit system */
            path = mprJoinPath(path, "VC/bin/x86_amd64/cl.exe");
        }
    } else {
        path = mprJoinPath(path, "VC/bin/cl.exe");
    }
    return path;
#else
    return getCompilerName(os, arch);
#endif
}

#endif /* ME_COM_ESP */
/*
    @copy   default

    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
/************************************************************************/
/*
    Start of file "src/mdb.c"
 */
/************************************************************************/
/*
    mdb.c -- In-Memory Embedded Database (MDB)

    WARNING: This is prototype code

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#include    "pcre.h"

#if ME_COM_ESP && ME_COM_MDB
/************************************* Local **********************************/

#define MDB_LOAD_BEGIN   1      /* Initial state */
#define MDB_LOAD_TABLE   2      /* Parsing a table */
#define MDB_LOAD_HINTS   3      /* Parsing hints */
#define MDB_LOAD_SCHEMA  4      /* Parsing schema */
#define MDB_LOAD_COL     5      /* Parsing column schema */ 
#define MDB_LOAD_DATA    6      /* Parsing data */
#define MDB_LOAD_FIELD   7      /* Parsing fields */

/*
    Operations for mdbReadWhere
 */
#define OP_ERR  -1              /* Illegal operation */
#define OP_EQ   0               /* "==" Equal operation */
#define OP_NEQ  0x2             /* "!=" Not equal operation */
#define OP_LT   0x4             /* "<" Less than operation */
#define OP_GT   0x8             /* ">" Greater than operation */
#define OP_LTE  0x10            /* ">=" Less than or equal operation */
#define OP_GTE  0x20            /* "<=" Greater than or equal operation */

/************************************ Forwards ********************************/

static void autoSave(Mdb *mdb, MdbTable *table);
static MdbCol *createCol(MdbTable *table, cchar *columnName);
static EdiRec *createRecFromRow(Edi *edi, MdbRow *row);
static MdbRow *createRow(Mdb *mdb, MdbTable *table);
static MdbCol *getCol(MdbTable *table, int col);
static MdbRow *getRow(MdbTable *table, int rid);
static MdbTable *getTable(Mdb *mdb, int tid);
static MdbSchema *growSchema(MdbTable *table);
static MdbCol *lookupField(MdbTable *table, cchar *columnName);
static int lookupRow(MdbTable *table, cchar *key);
static MdbTable *lookupTable(Mdb *mdb, cchar *tableName);
static void manageCol(MdbCol *col, int flags);
static void manageMdb(Mdb *mdb, int flags);
static void manageRow(MdbRow *row, int flags);
static void manageSchema(MdbSchema *schema, int flags);
static void manageTable(MdbTable *table, int flags);
static int parseOperation(cchar *operation);
static int updateFieldValue(MdbRow *row, MdbCol *col, cchar *value);
static int mdbAddColumn(Edi *edi, cchar *tableName, cchar *columnName, int type, int flags);
static int mdbAddIndex(Edi *edi, cchar *tableName, cchar *columnName, cchar *indexName);
static int mdbAddTable(Edi *edi, cchar *tableName);
static int mdbChangeColumn(Edi *edi, cchar *tableName, cchar *columnName, int type, int flags);
static void mdbClose(Edi *edi);
static EdiRec *mdbCreateRec(Edi *edi, cchar *tableName);
static int mdbDelete(cchar *path);
static MprList *mdbGetColumns(Edi *edi, cchar *tableName);
static int mdbGetColumnSchema(Edi *edi, cchar *tableName, cchar *columnName, int *type, int *flags, int *cid);
static MprList *mdbGetTables(Edi *edi);
static int mdbGetTableDimensions(Edi *edi, cchar *tableName, int *numRows, int *numCols);
static int mdbLoad(Edi *edi, cchar *path);
static int mdbLoadFromString(Edi *edi, cchar *string);
static int mdbLookupField(Edi *edi, cchar *tableName, cchar *fieldName);
static Edi *mdbOpen(cchar *path, int flags);
static EdiGrid *mdbQuery(Edi *edi, cchar *cmd, int argc, cchar **argv, va_list vargs);
static EdiField mdbReadField(Edi *edi, cchar *tableName, cchar *key, cchar *fieldName);
static EdiRec *mdbReadRec(Edi *edi, cchar *tableName, cchar *key);
static EdiGrid *mdbReadWhere(Edi *edi, cchar *tableName, cchar *columnName, cchar *operation, cchar *value);
static int mdbRemoveColumn(Edi *edi, cchar *tableName, cchar *columnName);
static int mdbRemoveIndex(Edi *edi, cchar *tableName, cchar *indexName);
static int mdbRemoveRec(Edi *edi, cchar *tableName, cchar *key);
static int mdbRemoveTable(Edi *edi, cchar *tableName);
static int mdbRenameTable(Edi *edi, cchar *tableName, cchar *newTableName);
static int mdbRenameColumn(Edi *edi, cchar *tableName, cchar *columnName, cchar *newColumnName);
static int mdbSave(Edi *edi);
static int mdbUpdateField(Edi *edi, cchar *tableName, cchar *key, cchar *fieldName, cchar *value);
static int mdbUpdateRec(Edi *edi, EdiRec *rec);

static EdiProvider MdbProvider = {
    "mdb",
    mdbAddColumn, mdbAddIndex, mdbAddTable, mdbChangeColumn, mdbClose, mdbCreateRec, mdbDelete, 
    mdbGetColumns, mdbGetColumnSchema, mdbGetTables, mdbGetTableDimensions, mdbLoad, mdbLookupField, mdbOpen, mdbQuery, 
    mdbReadField, mdbReadRec, mdbReadWhere, mdbRemoveColumn, mdbRemoveIndex, mdbRemoveRec, mdbRemoveTable, 
    mdbRenameTable, mdbRenameColumn, mdbSave, mdbUpdateField, mdbUpdateRec,
};

/************************************* Code ***********************************/

PUBLIC void mdbInit()
{
    ediAddProvider(&MdbProvider);
}


static Mdb *mdbAlloc(cchar *path, int flags)
{
    Mdb      *mdb;

    if ((mdb = mprAllocObj(Mdb, manageMdb)) == 0) {
        return 0;
    }
    mdb->edi.provider = &MdbProvider;
    mdb->edi.flags = flags;
    mdb->edi.path = sclone(path);
    mdb->edi.schemaCache = mprCreateHash(0, 0);
    mdb->edi.mutex = mprCreateLock();
    mdb->edi.validations = mprCreateHash(0, 0);
    return mdb;
}


static void manageMdb(Mdb *mdb, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(mdb->edi.path);
        mprMark(mdb->edi.schemaCache);
        mprMark(mdb->edi.errMsg);
        mprMark(mdb->edi.validations);
        mprMark(mdb->edi.mutex);
        mprMark(mdb->tables);
        /* Don't mark load fields */
    } else {
        mdbClose((Edi*) mdb);
    }
}


static void mdbClose(Edi *edi)
{
    Mdb     *mdb;
   
    mdb = (Mdb*) edi;
    autoSave(mdb, 0);
    mdb->tables = 0;
}


static EdiRec *mdbCreateRec(Edi *edi, cchar *tableName)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbCol      *col;
    EdiRec      *rec;
    EdiField    *fp;
    int         f, nfields;

    mdb = (Mdb*) edi;
    if ((table = lookupTable(mdb, tableName)) == 0) {
        return 0;
    }
    nfields = max(table->schema->ncols, 1);
    if ((rec = mprAllocMem(sizeof(EdiRec) + sizeof(EdiField) * nfields, MPR_ALLOC_MANAGER | MPR_ALLOC_ZERO)) == 0) {
        return 0;
    }
    mprSetManager(rec, (MprManager) ediManageEdiRec);

    rec->edi = edi;
    rec->tableName = table->name;
    rec->nfields = nfields;

    for (f = 0; f < nfields; f++) {
        col = getCol(table, f);
        fp = &rec->fields[f];
        fp->type = col->type;
        fp->name = col->name;
        fp->flags = col->flags;
    }
    return rec;
}


static int mdbDelete(cchar *path)
{
    return mprDeletePath(path);
}


static Edi *mdbOpen(cchar *source, int flags)
{
    Mdb     *mdb;

    if (flags & EDI_LITERAL) {
        flags |= EDI_NO_SAVE;
        if ((mdb = mdbAlloc("literal", flags)) == 0) {
            return 0;
        }
        if (mdbLoadFromString((Edi*) mdb, source) < 0) {
            return 0;
        }
    } else {
        if ((mdb = mdbAlloc(source, flags)) == 0) {
            return 0;
        }
        if (!mprPathExists(source, R_OK)) {
            if (flags & EDI_CREATE) {
                mdbSave((Edi*) mdb);
            } else {
                return 0;
            }
        }
        if (mdbLoad((Edi*) mdb, source) < 0) {
            return 0;
        }
    }
    return (Edi*) mdb;
}


static int mdbAddColumn(Edi *edi, cchar *tableName, cchar *columnName, int type, int flags)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbCol      *col;

    assert(edi);
    assert(tableName && *tableName);
    assert(columnName && *columnName);
    assert(type);

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupField(table, columnName)) != 0) {
        unlock(edi);
        return MPR_ERR_ALREADY_EXISTS;
    }
    if ((col = createCol(table, columnName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    col->type = type;
    col->flags = flags;
    if (flags & EDI_INDEX) {
        if (table->index) {
            mprError("Index already specified in table %s, replacing.", tableName);
        }
        if ((table->index = mprCreateHash(0, MPR_HASH_STATIC_VALUES | MPR_HASH_STABLE)) != 0) {
            table->indexCol = col;
        }
    }
    autoSave(mdb, table);
    unlock(edi);
    return 0;

}


/*
    IndexName is not implemented yet
 */
static int mdbAddIndex(Edi *edi, cchar *tableName, cchar *columnName, cchar *indexName)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbCol      *col;

    assert(edi);
    assert(tableName && *tableName);
    assert(columnName && *columnName);

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupField(table, columnName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if ((table->index = mprCreateHash(0, MPR_HASH_STATIC_VALUES | MPR_HASH_STABLE)) == 0) {
        unlock(edi);
        return MPR_ERR_MEMORY;
    }
    table->indexCol = col;
    col->flags |= EDI_INDEX;
    autoSave(mdb, table);
    unlock(edi);
    return 0;
}


static int mdbAddTable(Edi *edi, cchar *tableName)
{
    Mdb         *mdb;
    MdbTable    *table;

    assert(edi);
    assert(tableName && *tableName);

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) != 0) {
        unlock(edi);
        return MPR_ERR_ALREADY_EXISTS;
    }
    if ((table = mprAllocObj(MdbTable, manageTable)) == 0) {
        unlock(edi);
        return MPR_ERR_MEMORY;
    }
    if ((table->rows = mprCreateList(0, MPR_LIST_STABLE)) == 0) {
        unlock(edi);
        return MPR_ERR_MEMORY;
    }
    table->name = sclone(tableName);
    if (mdb->tables == 0) {
        mdb->tables = mprCreateList(0, MPR_LIST_STABLE);
    }
    if (!growSchema(table)) {
        unlock(edi);
        return MPR_ERR_MEMORY;
    }
    mprAddItem(mdb->tables, table);
    autoSave(mdb, lookupTable(mdb, tableName));
    unlock(edi);
    return 0;
}


static int mdbChangeColumn(Edi *edi, cchar *tableName, cchar *columnName, int type, int flags)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbCol      *col;

    assert(edi);
    assert(tableName && *tableName);
    assert(columnName && *columnName);
    assert(type);

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupField(table, columnName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    col->name = sclone(columnName);
    col->type = type;
    autoSave(mdb, table);
    unlock(edi);
    return 0;
}


/*
    Return a list of column names
 */
static MprList *mdbGetColumns(Edi *edi, cchar *tableName)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbSchema   *schema;
    MprList     *list;
    int         i;

    assert(edi);
    assert(tableName && *tableName);

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return 0;
    }
    schema = table->schema;
    assert(schema);
    list = mprCreateList(schema->ncols, MPR_LIST_STABLE);
    for (i = 0; i < schema->ncols; i++) {
        /* No need to clone */
        mprAddItem(list, schema->cols[i].name);
    }
    unlock(edi);
    return list;
}


/*
    Make a field. WARNING: the value is not cloned
 */
static EdiField makeFieldFromRow(MdbRow *row, MdbCol *col)
{
    EdiField    f;

    /* Note: the value is not cloned */
    f.value = row->fields[col->cid];
    f.type = col->type;
    f.name = col->name;
    f.flags = col->flags;
    f.valid = 1;
    return f;
}


static int mdbGetColumnSchema(Edi *edi, cchar *tableName, cchar *columnName, int *type, int *flags, int *cid)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbCol      *col;

    mdb = (Mdb*) edi;
    if (type) {
        *type = -1;
    }
    if (cid) {
        *cid = -1;
    }
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupField(table, columnName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if (type) {
        *type = col->type;
    }
    if (flags) {
        *flags = col->flags;
    }
    if (cid) {
        *cid = col->cid;
    }
    unlock(edi);
    return 0;
}


static MprList *mdbGetTables(Edi *edi)
{
    Mdb         *mdb;
    MprList     *list;
    MdbTable     *table;
    int         tid, ntables;

    mdb = (Mdb*) edi;
    lock(edi);
    list = mprCreateList(-1, MPR_LIST_STABLE);
    ntables = mprGetListLength(mdb->tables);
    for (tid = 0; tid < ntables; tid++) {
        table = mprGetItem(mdb->tables, tid);
        mprAddItem(list, table->name);
    }
    unlock(edi);
    return list;
}


static int mdbGetTableDimensions(Edi *edi, cchar *tableName, int *numRows, int *numCols)
{
    Mdb         *mdb;
    MdbTable    *table;

    mdb = (Mdb*) edi;
    lock(edi);
    if (numRows) {
        *numRows = 0;
    }
    if (numCols) {
        *numCols = 0;
    }
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if (numRows) {
        *numRows = mprGetListLength(table->rows);
    }
    if (numCols) {
        *numCols = table->schema->ncols;
    }
    unlock(edi);
    return 0;
}


static int mdbLoad(Edi *edi, cchar *path)
{
    cchar       *data;
    ssize       len;

    if ((data = mprReadPathContents(path, &len)) == 0) {
        return MPR_ERR_CANT_READ;
    }
    return mdbLoadFromString(edi, data);
}


static int mdbLookupField(Edi *edi, cchar *tableName, cchar *fieldName)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbCol      *col;

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupField(table, fieldName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    unlock(edi);
    return col->cid;
}


static EdiGrid *mdbQuery(Edi *edi, cchar *cmd, int argc, cchar **argv, va_list vargs)
{
    mprError("MDB does not implement ediQuery");
    return 0;
}


static EdiField mdbReadField(Edi *edi, cchar *tableName, cchar *key, cchar *fieldName)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbCol      *col;
    MdbRow      *row;
    EdiField    field, err;
    int         r;

    mdb = (Mdb*) edi;
    lock(edi);
    err.valid = 0;
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return err;
    }
    if ((col = lookupField(table, fieldName)) == 0) {
        unlock(edi);
        return err;
    }
    if ((r = lookupRow(table, key)) < 0) {
        unlock(edi);
        return err;
    }
    row = mprGetItem(table->rows, r);
    field = makeFieldFromRow(row, col);
    unlock(edi);
    return field;
}


static EdiRec *mdbReadRec(Edi *edi, cchar *tableName, cchar *key)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbRow      *row;
    EdiRec      *rec;
    int         r;

    mdb = (Mdb*) edi;
    rec = 0;

    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return 0;
    }
    if ((r = lookupRow(table, key)) < 0) {
        unlock(edi);
        return 0;
    }
    if ((row = mprGetItem(table->rows, r)) != 0) {
        rec = createRecFromRow(edi, row);
    }
    unlock(edi);
    return rec;
}



static bool matchRow(MdbCol *col, cchar *existing, int op, cchar *value)
{
    if (value == 0 || *value == '\0') {
        return 0;
    }
    switch (op) {
    case OP_EQ:
        if (smatch(existing, value)) {
            return 1;
        }
        break;
    case OP_NEQ:
        if (!smatch(existing, value)) {
            return 1;
        }
        break;
#if FUTURE
    case OP_LT:
    case OP_GT:
    case OP_LTE:
    case OP_GTE:
#endif
    default:
        assert(0);
    }
    return 0;
}


static EdiGrid *mdbReadWhere(Edi *edi, cchar *tableName, cchar *columnName, cchar *operation, cchar *value)
{
    Mdb         *mdb;
    EdiGrid     *grid;
    MdbTable    *table;
    MdbCol      *col;
    MdbRow      *row;
    int         nrows, next, op, r, count;

    assert(edi);
    assert(tableName && *tableName);

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return 0;
    }
    nrows = mprGetListLength(table->rows);
    if ((grid = ediCreateBareGrid(edi, tableName, nrows)) == 0) {
        unlock(edi);
        return 0;
    }
    grid->flags = EDI_GRID_READ_ONLY;
    if (columnName) {
        if ((col = lookupField(table, columnName)) == 0) {
            unlock(edi);
            return 0;
        }
        if ((op = parseOperation(operation)) < 0) {
            unlock(edi);
            return 0;
        }
        if (col->flags & EDI_INDEX && (op == OP_EQ)) {
            if ((r = lookupRow(table, value)) != 0) {
                row = getRow(table, r);
                grid->records[0] = createRecFromRow(edi, row);
                grid->nrecords = 1;
            }
        } else {
            grid->nrecords = count = 0;
            for (ITERATE_ITEMS(table->rows, row, next)) {
                if (!matchRow(col, row->fields[col->cid], op, value)) {
                    continue;
                }
                grid->records[count++] = createRecFromRow(edi, row);
                grid->nrecords = count;
            }
        }
    } else {
        for (ITERATE_ITEMS(table->rows, row, next)) {
            grid->records[next - 1] = createRecFromRow(edi, row);
        }
        grid->nrecords = next;
    }
    unlock(edi);
    return grid;
}


static int mdbRemoveColumn(Edi *edi, cchar *tableName, cchar *columnName)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbSchema   *schema;
    MdbCol      *col;
    int         c;

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupField(table, columnName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if (table->indexCol == col) {
        table->index = 0;
        table->indexCol = 0;
    }
    if (table->keyCol == col) {
        table->keyCol = 0;
    }
    schema = table->schema;
    assert(schema);
    for (c = col->cid; c < schema->ncols; c++) {
        schema->cols[c] = schema->cols[c + 1];
    }
    schema->ncols--;
    schema->cols[schema->ncols].name = 0;
    assert(schema->ncols >= 0);
    autoSave(mdb, table);
    unlock(edi);
    return 0;
}


static int mdbRemoveIndex(Edi *edi, cchar *tableName, cchar *indexName)
{
    Mdb         *mdb;
    MdbTable    *table;

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    table->index = 0;
    if (table->indexCol) {
        table->indexCol->flags &= ~EDI_INDEX;
        table->indexCol = 0;
        autoSave(mdb, table);
    }
    unlock(edi);
    return 0;
}


static int mdbRemoveRec(Edi *edi, cchar *tableName, cchar *key)
{
    Mdb         *mdb;
    MdbTable    *table;
    MprKey      *kp;
    int         r, rc, value;

    assert(edi);
    assert(tableName && *tableName);
    assert(key && *key);

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if ((r = lookupRow(table, key)) < 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    rc = mprRemoveItemAtPos(table->rows, r);
    if (table->index) {
        mprRemoveKey(table->index, key);
        for (ITERATE_KEYS(table->index, kp)) {
            value = (int) PTOL(kp->data);
            if (value >= r) {
                mprAddKey(table->index, kp->key, LTOP(value - 1));
            }
        }
    }
    autoSave(mdb, table);
    unlock(edi);
    return rc;
}


static int mdbRemoveTable(Edi *edi, cchar *tableName)
{
    Mdb         *mdb;
    MdbTable    *table;
    int         next;

    mdb = (Mdb*) edi;
    lock(edi);
    for (ITERATE_ITEMS(mdb->tables, table, next)) {
        if (smatch(table->name, tableName)) {
            mprRemoveItem(mdb->tables, table);
            autoSave(mdb, table);
            unlock(edi);
            return 0;
        }
    }
    unlock(edi);
    return MPR_ERR_CANT_FIND;
}


static int mdbRenameTable(Edi *edi, cchar *tableName, cchar *newTableName)
{
    Mdb         *mdb;
    MdbTable    *table;

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    table->name = sclone(newTableName);
    autoSave(mdb, table);
    unlock(edi);
    return 0;
}


static int mdbRenameColumn(Edi *edi, cchar *tableName, cchar *columnName, cchar *newColumnName)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbCol      *col;

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupField(table, columnName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    col->name = sclone(newColumnName);
    autoSave(mdb, table);
    unlock(edi);
    return 0;
}


static int mdbUpdateField(Edi *edi, cchar *tableName, cchar *key, cchar *fieldName, cchar *value)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbRow      *row;
    MdbCol      *col;
    int         r;

    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if ((col = lookupField(table, fieldName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if ((r = lookupRow(table, key)) < 0) {
        row = createRow(mdb, table);
    }
    if ((row = getRow(table, r)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    updateFieldValue(row, col, value);
    autoSave(mdb, table);
    unlock(edi);
    return 0;
}


static int mdbUpdateRec(Edi *edi, EdiRec *rec)
{
    Mdb         *mdb;
    MdbTable    *table;
    MdbRow      *row;
    MdbCol      *col;
    int         f, r;

    if (!ediValidateRec(rec)) {
        return MPR_ERR_CANT_WRITE;
    }
    mdb = (Mdb*) edi;
    lock(edi);
    if ((table = lookupTable(mdb, rec->tableName)) == 0) {
        unlock(edi);
        return MPR_ERR_CANT_FIND;
    }
    if (rec->id == 0 || (r = lookupRow(table, rec->id)) < 0) {
        row = createRow(mdb, table);
    } else {
        if ((row = getRow(table, r)) == 0) {
            unlock(edi);
            return MPR_ERR_CANT_FIND;
        }
    }
    for (f = 0; f < rec->nfields; f++) {
        if ((col = getCol(table, f)) != 0) {
            updateFieldValue(row, col, rec->fields[f].value);
        }
    }
    autoSave(mdb, table);
    unlock(edi);
    return 0;
}


/******************************** Database Loading ***************************/

static void clearLoadState(Mdb *mdb)
{
    mdb->loadNcols = 0;
    mdb->loadCol = 0;
    mdb->loadRow = 0;
}


static void pushState(Mdb *mdb, int state)
{
    mprPushItem(mdb->loadStack, LTOP(state));
    mdb->loadState = state;
}


static void popState(Mdb *mdb)
{
    mprPopItem(mdb->loadStack);
    mdb->loadState = (int) PTOL(mprGetLastItem(mdb->loadStack));
    assert(mdb->loadState > 0);
}


static int checkMdbState(MprJsonParser *jp, cchar *name, bool leave)
{
    Mdb     *mdb;

    mdb = jp->data;
    if (leave) {
        popState(mdb);
        return 0;
    }
    switch (mdb->loadState) {
    case MDB_LOAD_BEGIN:
        if (mdbAddTable((Edi*) mdb, name) < 0) {
            return MPR_ERR_MEMORY;
        }
        mdb->loadTable = lookupTable(mdb, name);
        clearLoadState(mdb);
        pushState(mdb, MDB_LOAD_TABLE);
        break;
        
    case MDB_LOAD_TABLE:
        if (smatch(name, "hints")) {
            pushState(mdb, MDB_LOAD_HINTS);
        } else if (smatch(name, "schema")) {
            pushState(mdb, MDB_LOAD_SCHEMA);
        } else if (smatch(name, "data")) {
            pushState(mdb, MDB_LOAD_DATA);
        } else {
            mprSetJsonError(jp, "Bad property '%s'", name);
            return MPR_ERR_BAD_FORMAT;
        }
        break;
    
    case MDB_LOAD_SCHEMA:
        if ((mdb->loadCol = createCol(mdb->loadTable, name)) == 0) {
            mprSetJsonError(jp, "Cannot create '%s' column", name);
            return MPR_ERR_MEMORY;
        }
        pushState(mdb, MDB_LOAD_COL);
        break;

    case MDB_LOAD_DATA:
        if ((mdb->loadRow = createRow(mdb, mdb->loadTable)) == 0) {
            return MPR_ERR_MEMORY;
        }
        mdb->loadCid = 0;
        pushState(mdb, MDB_LOAD_FIELD);
        break;

    case MDB_LOAD_HINTS:
    case MDB_LOAD_COL:
    case MDB_LOAD_FIELD:
        pushState(mdb, mdb->loadState);
        break;

    default:
        mprSetJsonError(jp, "Potential corrupt data. Bad state");
        return MPR_ERR_BAD_FORMAT;
    }
    return 0;
}


static int setMdbValue(MprJsonParser *parser, MprJson *obj, cchar *name, MprJson *child)
{
    Mdb         *mdb;
    MdbCol      *col;
    cchar       *value;

    mdb = parser->data;
    value = child->value;
    
    switch (mdb->loadState) {
    case MDB_LOAD_BEGIN:
    case MDB_LOAD_TABLE:
    case MDB_LOAD_SCHEMA:
    case MDB_LOAD_DATA:
        break;

    case MDB_LOAD_HINTS:
        if (smatch(name, "ncols")) {
            mdb->loadNcols = atoi(value);
        } else {
            mprSetJsonError(parser, "Unknown hint '%s'", name);
            return MPR_ERR_BAD_FORMAT;
        }
        break;

    case MDB_LOAD_COL:
        if (smatch(name, "index")) {
            mdbAddIndex((Edi*) mdb, mdb->loadTable->name, mdb->loadCol->name, NULL);
        } else if (smatch(name, "type")) {
            if ((mdb->loadCol->type = ediParseTypeString(value)) <= 0) {
                mprSetJsonError(parser, "Bad column type %s", value);
                return MPR_ERR_BAD_FORMAT;
            }
        } else if (smatch(name, "key")) {
            mdb->loadCol->flags |= EDI_KEY;
            mdb->loadTable->keyCol = mdb->loadCol;
        } else if (smatch(name, "autoinc")) {
            mdb->loadCol->flags |= EDI_AUTO_INC;
        } else if (smatch(name, "foreign")) {
            mdb->loadCol->flags |= EDI_FOREIGN;
        } else if (smatch(name, "notnull")) {
            mdb->loadCol->flags |= EDI_NOT_NULL;
        } else {
            mprSetJsonError(parser, "Bad property '%s' in column definition", name);
            return MPR_ERR_BAD_FORMAT;
        }
        break;

    case MDB_LOAD_FIELD:
        if ((col = getCol(mdb->loadTable, mdb->loadCid++)) == 0) {
            mprSetJsonError(parser, "Bad state '%d' in setMdbValue, column %s,  potential corrupt data", mdb->loadState, name);
            return MPR_ERR_BAD_FORMAT;
        }
        updateFieldValue(mdb->loadRow, col, value);
        break;

    default:
        mprSetJsonError(parser, "Bad state '%d' in setMdbValue potential corrupt data", mdb->loadState);
        return MPR_ERR_BAD_FORMAT;
    }
    return 0;
}


static int mdbLoadFromString(Edi *edi, cchar *str)
{
    Mdb             *mdb;
    MprJson         *obj;
    MprJsonCallback cb;

    mdb = (Mdb*) edi;
    mdb->edi.flags |= EDI_SUPPRESS_SAVE;
    mdb->edi.flags |= MDB_LOADING;
    mdb->loadStack = mprCreateList(0, MPR_LIST_STABLE);
    pushState(mdb, MDB_LOAD_BEGIN);

    memset(&cb, 0, sizeof(cb));
    cb.checkBlock = checkMdbState;
    cb.setValue = setMdbValue;

    obj = mprParseJsonEx(str, &cb, mdb, 0, 0);
    mdb->edi.flags &= ~MDB_LOADING;
    mdb->edi.flags &= ~EDI_SUPPRESS_SAVE;
    mdb->loadStack = 0;
    if (obj == 0) {
        return MPR_ERR_CANT_LOAD;
    }
    return 0;
}


/******************************** Database Saving ****************************/

static void autoSave(Mdb *mdb, MdbTable *table)
{
    assert(mdb);

    if (mdb->edi.flags & EDI_NO_SAVE) {
        return;
    }
    if (mdb->edi.flags & EDI_AUTO_SAVE && !(mdb->edi.flags & EDI_SUPPRESS_SAVE)) {
        if (mdbSave((Edi*) mdb) < 0) {
            mprError("Cannot save database %s", mdb->edi.path);
        }
    }
}


/*
    Must be called locked
 */
static int mdbSave(Edi *edi)
{
    Mdb         *mdb;
    MdbSchema   *schema;
    MdbTable    *table;
    MdbRow      *row;
    MdbCol      *col;
    cchar       *value, *path, *cp;
    char        *npath, *bak, *type;
    MprFile     *out;
    int         cid, rid, tid, ntables, nrows;

    mdb = (Mdb*) edi;
    if (mdb->edi.flags & EDI_NO_SAVE) {
        return MPR_ERR_BAD_STATE;
    }
    path = mdb->edi.path;
    if (path == 0) {
        mprError("No database path specified");
        return MPR_ERR_BAD_ARGS;
    }
    npath = mprReplacePathExt(path, "new");
    if ((out = mprOpenFile(npath, O_WRONLY | O_TRUNC | O_CREAT | O_BINARY, 0664)) == 0) {
        mprError("Cannot open database %s", npath);
        return 0;
    }
    mprEnableFileBuffering(out, 0, 0);
    mprWriteFileFmt(out, "{\n");

    ntables = mprGetListLength(mdb->tables);
    for (tid = 0; tid < ntables; tid++) {
        table = getTable(mdb, tid);
        schema = table->schema;
        assert(schema);
        mprWriteFileFmt(out, "    '%s': {\n", table->name);
        mprWriteFileFmt(out, "        hints: {\n            ncols: %d\n        },\n", schema->ncols);
        mprWriteFileString(out, "        schema: {\n");
        /* Skip the id which is always the first column */
        for (cid = 0; cid < schema->ncols; cid++) {
            col = getCol(table, cid);
            type = ediGetTypeString(col->type);
            mprWriteFileFmt(out, "            '%s': { type: '%s'", col->name, type);
            if (col->flags & EDI_AUTO_INC) {
                mprWriteFileString(out, ", autoinc: true");
            }
            if (col->flags & EDI_INDEX) {
                mprWriteFileString(out, ", index: true");
            }
            if (col->flags & EDI_KEY) {
                mprWriteFileString(out, ", key: true");
            }
            if (col->flags & EDI_FOREIGN) {
                mprWriteFileString(out, ", foreign: true");
            }
            if (col->flags & EDI_NOT_NULL) {
                mprWriteFileString(out, ", notnull: true");
            }
            mprWriteFileString(out, " },\n");
        }
        mprWriteFileString(out, "        },\n");
        mprWriteFileString(out, "        data: [\n");

        nrows = mprGetListLength(table->rows);
        for (rid = 0; rid < nrows; rid++) {
            mprWriteFileString(out, "            [ ");
            row = getRow(table, rid);
            for (cid = 0; cid < schema->ncols; cid++) {
                col = getCol(table, cid);
                value = row->fields[col->cid];
                if (value == 0 && col->flags & EDI_AUTO_INC) {
                    row->fields[col->cid] = itos(++col->lastValue);
                }
                if (value == 0) {
                    mprWriteFileFmt(out, "null, ");
                } else if (col->type == EDI_TYPE_STRING || col->type == EDI_TYPE_TEXT) {
                    mprWriteFile(out, "'", 1);
                    for (cp = value; *cp; cp++) {
                        if (*cp == '\'' || *cp == '\\') {
                            mprWriteFile(out, "\\", 1);
                        }
                        mprWriteFile(out, cp, 1);
                    }
                    mprWriteFile(out, "',", 2);
                } else {
                    for (cp = value; *cp; cp++) {
                        if (*cp == '\'' || *cp == '\\') {
                            mprWriteFile(out, "\\", 1);
                        }
                        mprWriteFile(out, cp, 1);
                    }
                    mprWriteFile(out, ",", 1);
                }
            }
            mprWriteFileString(out, "],\n");
        }
        mprWriteFileString(out, "        ],\n    },\n");
    }
    mprWriteFileString(out, "}\n");
    mprCloseFile(out);

    bak = mprReplacePathExt(path, "bak");
    mprDeletePath(bak);
    if (mprPathExists(path, R_OK) && rename(path, bak) < 0) {
        mprError("Cannot rename %s to %s", path, bak);
        return MPR_ERR_CANT_WRITE;
    }
    if (rename(npath, path) < 0) {
        mprError("Cannot rename %s to %s", npath, path);
        /* Restore backup */
        rename(bak, path);
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}


/********************************* Table Operations **************************/

static MdbTable *getTable(Mdb *mdb, int tid)
{
    int         ntables;

    ntables = mprGetListLength(mdb->tables);
    if (tid < 0 || tid >= ntables) {
        return 0;
    }
    return mprGetItem(mdb->tables, tid);
}


static MdbTable *lookupTable(Mdb *mdb, cchar *tableName)
{
    MdbTable     *table;
    int         tid, ntables;

    ntables = mprGetListLength(mdb->tables);
    for (tid = 0; tid < ntables; tid++) {
        table = mprGetItem(mdb->tables, tid);
        if (smatch(table->name, tableName)) {
            return table;
        }
    }
    return 0;
}


static void manageTable(MdbTable *table, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(table->name);
        mprMark(table->schema);
        mprMark(table->index);
        mprMark(table->rows);
    }
}


static int lookupRow(MdbTable *table, cchar *key)
{
    MprKey      *kp;
    MdbRow      *row;
    int         nrows, r, keycol;

    if (table->index) {
        if ((kp = mprLookupKeyEntry(table->index, key)) != 0) {
            return (int) PTOL(kp->data);
        } 
    } else {
        nrows = mprGetListLength(table->rows);
        keycol = table->keyCol ? table->keyCol->cid : 0;
        for (r = 0; r < nrows; r++) {
            row = mprGetItem(table->rows, r);
            if (smatch(row->fields[keycol], key)) {
                return r;
            }
        }
    }
    return -1;
}


/********************************* Schema / Col ****************************/

static MdbSchema *growSchema(MdbTable *table)
{
    if (table->schema == 0) {
        if ((table->schema = mprAllocMem(sizeof(MdbSchema) + 
                sizeof(MdbCol) * MDB_INCR, MPR_ALLOC_MANAGER | MPR_ALLOC_ZERO)) == 0) {
            return 0;
        }
        mprSetManager(table->schema, (MprManager) manageSchema);
        table->schema->capacity = MDB_INCR;

    } else if (table->schema->ncols >= table->schema->capacity) {
        if ((table->schema = mprRealloc(table->schema, sizeof(MdbSchema) + 
                (sizeof(MdbCol) * (table->schema->capacity + MDB_INCR)))) == 0) {
            return 0;
        }
        table->schema->capacity += MDB_INCR;
    }
    return table->schema;
}


static MdbCol *createCol(MdbTable *table, cchar *columnName)
{
    MdbSchema    *schema;
    MdbCol       *col;

    if ((col = lookupField(table, columnName)) != 0) {
        return 0;
    }
    if ((schema = growSchema(table)) == 0) {
        return 0;
    }
    col = &schema->cols[schema->ncols];
    col->cid = schema->ncols++;
    col->name = sclone(columnName);
    return col;
}


static void manageSchema(MdbSchema *schema, int flags) 
{
    int     i;

    if (flags & MPR_MANAGE_MARK) {
        for (i = 0; i < schema->ncols; i++) {
            manageCol(&schema->cols[i], flags);
        }
    }
}


static void manageCol(MdbCol *col, int flags) 
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(col->name);
    }
}


static MdbCol *getCol(MdbTable *table, int col)
{
    if (col < 0 || col >= table->schema->ncols) {
        return 0;
    }
    return &table->schema->cols[col];
}


static MdbCol *lookupField(MdbTable *table, cchar *columnName)
{
    MdbSchema    *schema;
    MdbCol       *col;

    if ((schema = growSchema(table)) == 0) {
        return 0;
    }
    for (col = schema->cols; col < &schema->cols[schema->ncols]; col++) {
        if (smatch(col->name, columnName)) {
            return col;
        }
    }
    return 0;
}


/********************************* Row Operations **************************/

static MdbRow *createRow(Mdb *mdb, MdbTable *table)
{
    MdbRow      *row;
    int         ncols;

    ncols = max(table->schema->ncols, 1);
    if ((row = mprAllocMem(sizeof(MdbRow) + sizeof(EdiField) * ncols, MPR_ALLOC_MANAGER | MPR_ALLOC_ZERO)) == 0) {
        return 0;
    }
    mprSetManager(row, (MprManager) manageRow);
    row->table = table;
    row->nfields = ncols;
    row->rid = mprAddItem(table->rows, row);
    return row;
}


static void manageRow(MdbRow *row, int flags)
{
    int     fid;

    if (flags & MPR_MANAGE_MARK) {
        for (fid = 0; fid < row->nfields; fid++) {
            mprMark(row->fields[fid]);
        }
    }
}


static MdbRow *getRow(MdbTable *table, int rid)
{
    int     nrows;

    nrows = mprGetListLength(table->rows);
    if (rid < 0 || rid > nrows) {
        return 0;
    }
    return mprGetItem(table->rows, rid);
}

/********************************* Field Operations ************************/

static cchar *mapMdbValue(cchar *value, int type)
{
    MprTime     time;

    if (value == 0) {
        return value;
    }
    switch (type) {
    case EDI_TYPE_DATE:
        if (!snumber(value)) {
            mprParseTime(&time, value, MPR_UTC_TIMEZONE, 0);
            value = itos(time);
        }
        break;

    case EDI_TYPE_BINARY:
    case EDI_TYPE_BOOL:
    case EDI_TYPE_FLOAT:
    case EDI_TYPE_INT:
    case EDI_TYPE_STRING:
    case EDI_TYPE_TEXT:
    default:
        break;
    }
    return sclone(value);
}

static int updateFieldValue(MdbRow *row, MdbCol *col, cchar *value)
{
    MdbTable    *table;
    cchar       *key;
    
    assert(row);
    assert(col);

    table = row->table;
    if (col->flags & EDI_INDEX) {
        if ((key = row->fields[col->cid]) != 0) {
            mprRemoveKey(table->index, key);
        }
    } else {
        key = 0;
    }
    if (col->flags & EDI_AUTO_INC) {
        if (value == 0) {
            row->fields[col->cid] = value = itos(++col->lastValue);
        } else {
            row->fields[col->cid] = sclone(value);
            col->lastValue = max(col->lastValue, (int64) stoi(value));
        }
    } else {
        row->fields[col->cid] = mapMdbValue(value, col->type);
    }
    if (col->flags & EDI_INDEX && value) {
        mprAddKey(table->index, value, LTOP(row->rid));
    }
    return 0;
}

/*********************************** Support *******************************/
/*
    Optimized record creation
 */
static EdiRec *createRecFromRow(Edi *edi, MdbRow *row)
{
    EdiRec  *rec;
    MdbCol  *col;
    int     c;

    if ((rec = mprAllocMem(sizeof(EdiRec) + sizeof(EdiField) * row->nfields, MPR_ALLOC_MANAGER | MPR_ALLOC_ZERO)) == 0) {
        return 0;
    }
    mprSetManager(rec, (MprManager) ediManageEdiRec);
    rec->edi = edi;
    rec->tableName = row->table->name;
    rec->id = row->fields[0];
    rec->nfields = row->nfields;
    for (c = 0; c < row->nfields; c++) {
        col = getCol(row->table, c);
        rec->fields[c] = makeFieldFromRow(row, col);
    }
    return rec;
}


static int parseOperation(cchar *operation)
{
    switch (*operation) {
    case '=':
        if (smatch(operation, "==")) {
            return OP_EQ;
        }
        break;
    case '!':
        if (smatch(operation, "=!")) {
            return OP_EQ;
        }
        break;
    case '<':
        if (smatch(operation, "<")) {
            return OP_LT;
        } else if (smatch(operation, "<=")) {
            return OP_LTE;
        }
        break;
    case '>':
        if (smatch(operation, ">")) {
            return OP_GT;
        } else if (smatch(operation, ">=")) {
            return OP_GTE;
        }
    }
    mprError("Unknown read operation '%s'", operation);
    return OP_ERR;
}

#else
/* To prevent ar/ranlib warnings */
PUBLIC void mdbDummy() {}

#endif /* ME_COM_ESP && ME_COM_MDB */
/*
    @copy   default

    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
/************************************************************************/
/*
    Start of file "src/sdb.c"
 */
/************************************************************************/
/*
    sdb.c -- SQLite Database (SDB)

    Copyright (c) All Rights Reserved. See copyright notice at the bottom of the file.
 */

/********************************** Includes **********************************/

#if ME_COM_ESP && ME_COM_SQLITE
 #include    "sqlite3.h"

#ifndef ME_MAX_SQLITE_MEM
    #define ME_MAX_SQLITE_MEM      (2*1024*1024)   /**< Maximum buffering for Sqlite */
#endif
#ifndef ME_MAX_SQLITE_DURATION
    #define ME_MAX_SQLITE_DURATION 30000           /**< Database busy timeout */
#endif

/************************************* Local **********************************/

typedef struct Sdb {
    Edi             edi;            /**< EDI database interface structure */
    sqlite3         *db;            /**< SQLite database handle */
    MprHash         *schemas;       /**< Table schemas */
} Sdb;

static int sqliteInitialized;
static void initSqlite();

#if KEEP
static char *DataTypeToSqlType[] = {
    "binary":       "blob",
    "boolean":      "tinyint",
    "date":         "date",
    "datetime":     "datetime",
    "decimal":      "decimal",
    "float":        "float",
    "integer":      "int",
    "number":       "decimal",
    "string":       "varchar",
    "text":         "text",
    "time":         "time",
    "timestamp":    "datetime",
    0, 0, 
};
#endif

static char *dataTypeToSqlType[] = {
                            0,
    /* EDI_TYPE_BINARY */   "BLOB",
    /* EDI_TYPE_BOOL */     "TINYINT",      //  "INTEGER",
    /* EDI_TYPE_DATE */     "DATE",         //  "TEXT",
    /* EDI_TYPE_FLOAT */    "FLOAT",        //  "REAL",
    /* EDI_TYPE_INT */      "INTEGER",
    /* EDI_TYPE_STRING */   "STRING",       //  "TEXT",
    /* EDI_TYPE_TEXT */     "TEXT",
                            0,
};

/************************************ Forwards ********************************/

static EdiRec *createBareRec(Edi *edi, cchar *tableName, int nfields);
static EdiField makeRecField(cchar *value, cchar *name, int type);
static void manageSdb(Sdb *sdb, int flags);
static int mapSqliteTypeToEdiType(int type);
static cchar *mapToSqlType(int type);
static int mapToEdiType(cchar *type);
static EdiGrid *query(Edi *edi, cchar *cmd, ...);
static EdiGrid *queryArgv(Edi *edi, cchar *cmd, int argc, cchar **argv, ...);
static EdiGrid *queryv(Edi *edi, cchar *cmd, int argc, cchar **argv, va_list args);
static int sdbAddColumn(Edi *edi, cchar *tableName, cchar *columnName, int type, int flags);
static int sdbAddIndex(Edi *edi, cchar *tableName, cchar *columnName, cchar *indexName);
static int sdbAddTable(Edi *edi, cchar *tableName);
static int sdbChangeColumn(Edi *edi, cchar *tableName, cchar *columnName, int type, int flags);
static void sdbClose(Edi *edi);
static EdiRec *sdbCreateRec(Edi *edi, cchar *tableName);
static int sdbDelete(cchar *path);
static void sdbError(Edi *edi, cchar *fmt, ...);
static int sdbRemoveRec(Edi *edi, cchar *tableName, cchar *key);
static MprList *sdbGetColumns(Edi *edi, cchar *tableName);
static int sdbGetColumnSchema(Edi *edi, cchar *tableName, cchar *columnName, int *type, int *flags, int *cid);
static MprList *sdbGetTables(Edi *edi);
static int sdbGetTableDimensions(Edi *edi, cchar *tableName, int *numRows, int *numCols);
static int sdbLookupField(Edi *edi, cchar *tableName, cchar *fieldName);
static Edi *sdbOpen(cchar *path, int flags);
PUBLIC EdiGrid *sdbQuery(Edi *edi, cchar *cmd, int argc, cchar **argv, va_list vargs);
static EdiField sdbReadField(Edi *edi, cchar *tableName, cchar *key, cchar *fieldName);
static EdiRec *sdbReadRec(Edi *edi, cchar *tableName, cchar *key);
static EdiGrid *sdbReadWhere(Edi *edi, cchar *tableName, cchar *columnName, cchar *operation, cchar *value);
static int sdbRemoveColumn(Edi *edi, cchar *tableName, cchar *columnName);
static int sdbRemoveIndex(Edi *edi, cchar *tableName, cchar *indexName);
static int sdbRemoveTable(Edi *edi, cchar *tableName);
static int sdbRenameTable(Edi *edi, cchar *tableName, cchar *newTableName);
static int sdbRenameColumn(Edi *edi, cchar *tableName, cchar *columnName, cchar *newColumnName);
static int sdbSave(Edi *edi);
static void sdbTrace(Edi *edi, int level, cchar *fmt, ...);
static int sdbUpdateField(Edi *edi, cchar *tableName, cchar *key, cchar *fieldName, cchar *value);
static int sdbUpdateRec(Edi *edi, EdiRec *rec);
static bool validName(cchar *str);

static EdiProvider SdbProvider = {
    "sdb",
    sdbAddColumn, sdbAddIndex, sdbAddTable, sdbChangeColumn, sdbClose, sdbCreateRec, sdbDelete, 
    sdbGetColumns, sdbGetColumnSchema, sdbGetTables, sdbGetTableDimensions, NULL, sdbLookupField, sdbOpen, sdbQuery, 
    sdbReadField, sdbReadRec, sdbReadWhere, sdbRemoveColumn, sdbRemoveIndex, sdbRemoveRec, sdbRemoveTable, 
    sdbRenameTable, sdbRenameColumn, sdbSave, sdbUpdateField, sdbUpdateRec,
};

/************************************* Code ***********************************/

PUBLIC void sdbInit()
{
    ediAddProvider(&SdbProvider);
}


static Sdb *sdbCreate(cchar *path, int flags)
{
    Sdb      *sdb;

    assert(path && *path);

    initSqlite();
    if ((sdb = mprAllocObj(Sdb, manageSdb)) == 0) {
        return 0;
    }
    sdb->edi.flags = flags;
    sdb->edi.provider = &SdbProvider;
    sdb->edi.path = sclone(path);
    sdb->edi.schemaCache = mprCreateHash(0, 0);
    sdb->edi.validations = mprCreateHash(0, 0);
    sdb->edi.mutex = mprCreateLock();
    sdb->schemas = mprCreateHash(0, MPR_HASH_STABLE);
    return sdb;
}


static void manageSdb(Sdb *sdb, int flags)
{
    if (flags & MPR_MANAGE_MARK) {
        mprMark(sdb->edi.path);
        mprMark(sdb->edi.schemaCache);
        mprMark(sdb->edi.errMsg);
        mprMark(sdb->edi.mutex);
        mprMark(sdb->edi.validations);
        mprMark(sdb->schemas);

    } else if (flags & MPR_MANAGE_FREE) {
        sdbClose((Edi*) sdb);
    }
}


static void sdbClose(Edi *edi)
{
    Sdb     *sdb;

    assert(edi);

    sdb = (Sdb*) edi;
    if (sdb->db) {
        sqlite3_close(sdb->db);
        sdb->db = 0;
    }
}


static void removeSchema(Edi *edi, cchar *tableName)
{
    mprRemoveKey(((Sdb*) edi)->schemas, tableName);
}


static EdiRec *getSchema(Edi *edi, cchar *tableName)
{
    Sdb         *sdb;
    EdiRec      *rec, *schema;
    EdiField    *fp;
    EdiGrid     *grid;
    int         r;

    sdb = (Sdb*) edi;
    if (!validName(tableName)) {
        return 0;
    }
    if ((schema = mprLookupKey(sdb->schemas, tableName)) != 0) {
        return schema;
    }
    /*
        Each result row represents an EDI column: CID, Name, Type, NotNull, Dflt_value, PK
     */
    if ((grid = query(edi, sfmt("PRAGMA table_info(%s);", tableName), NULL)) == 0) {
        return 0;
    }
    schema = createBareRec(edi, tableName, grid->nrecords);
    for (r = 0; r < grid->nrecords; r++) {
        rec = grid->records[r];
        fp = &schema->fields[r];
        fp->name = rec->fields[1].value;
        fp->type = mapToEdiType(rec->fields[2].value);
        if (rec->fields[5].value && rec->fields[5].value[0] == '1') {
            fp->flags = EDI_KEY;
        }
    }
    mprAddKey(sdb->schemas, tableName, schema);
    return schema;
}


static EdiRec *sdbCreateRec(Edi *edi, cchar *tableName)
{
    EdiRec  *rec, *schema;
    int     i;

    schema = getSchema(edi, tableName);
    if ((rec = mprAllocMem(sizeof(EdiRec) + sizeof(EdiField) * schema->nfields, MPR_ALLOC_MANAGER | MPR_ALLOC_ZERO)) == 0) {
        return 0;
    }
    mprSetManager(rec, (MprManager) ediManageEdiRec);
    rec->edi = edi;
    rec->tableName = sclone(tableName);
    rec->nfields = schema->nfields;
    for (i = 0; i < schema->nfields; i++) {
        rec->fields[i].name = schema->fields[i].name;
        rec->fields[i].type = schema->fields[i].type;
        rec->fields[i].flags = schema->fields[i].flags;
    }
    return rec;
}


static int sdbDelete(cchar *path)
{
    assert(path && *path);
    return mprDeletePath(path);
}


static Edi *sdbOpen(cchar *path, int flags)
{
    Sdb     *sdb;

    assert(path && *path);
    if ((sdb = sdbCreate(path, flags)) == 0) {
        return 0;
    }
    if (mprPathExists(path, R_OK) || (flags & EDI_CREATE)) {
        if (sqlite3_open(path, &sdb->db) != SQLITE_OK) {
            mprError("Cannot open database %s", path);
            return 0;
        }
        sqlite3_soft_heap_limit(ME_MAX_SQLITE_MEM);
        sqlite3_busy_timeout(sdb->db, ME_MAX_SQLITE_DURATION);
    } else {
        return 0;
    }
    return (Edi*) sdb;
}


static int sdbAddColumn(Edi *edi, cchar *tableName, cchar *columnName, int type, int flags)
{
    assert(edi);
    assert(tableName && *tableName);
    assert(columnName && *columnName);
    assert(type > 0);

    if (!validName(tableName) || !validName(columnName)) {
        return MPR_ERR_BAD_ARGS;
    }
    if (sdbLookupField(edi, tableName, columnName) >= 0) {
        /* Already exists */
        return 0;
    }
    removeSchema(edi, tableName);
    /*
        The field types are used for the SQLite column affinity settings
     */
    if (query(edi, sfmt("ALTER TABLE %s ADD %s %s", tableName, columnName, mapToSqlType(type)), NULL) == 0) {
        return MPR_ERR_CANT_CREATE;
    }
    return 0;
}


static int sdbAddIndex(Edi *edi, cchar *tableName, cchar *columnName, cchar *indexName)
{
    assert(edi);
    assert(tableName && *tableName);
    assert(columnName && *columnName);
    assert(indexName && *indexName);

    if (!validName(tableName) || !validName(columnName) || !validName(indexName)) {
        return MPR_ERR_BAD_ARGS;
    }
    return query(edi, sfmt("CREATE INDEX %s ON %s (%s);", indexName, tableName, columnName), NULL) != 0;
}


static int sdbAddTable(Edi *edi, cchar *tableName)
{
    assert(edi);
    assert(tableName && *tableName);

    if (!validName(tableName)) {
        return MPR_ERR_BAD_ARGS;
    }
    if (query(edi, sfmt("DROP TABLE IF EXISTS %s;", tableName), NULL) == 0) {
        return MPR_ERR_CANT_DELETE;
    }
    /*
        SQLite cannot add a primary key after the table is created
     */
    removeSchema(edi, tableName);
    return query(edi, sfmt("CREATE TABLE %s (id INTEGER PRIMARY KEY AUTOINCREMENT NOT NULL);", tableName), NULL) != 0;
}


static int sdbChangeColumn(Edi *edi, cchar *tableName, cchar *columnName, int type, int flags)
{
    mprError("SDB does not support changing columns");
    return MPR_ERR_BAD_STATE;
}


static MprList *sdbGetColumns(Edi *edi, cchar *tableName)
{
    MprList     *result;
    EdiRec      *schema;
    int         c;

    assert(edi);
    assert(tableName && *tableName);
    
    if ((schema = getSchema(edi, tableName)) == 0) {
        return 0;
    }
    if ((result = mprCreateList(0, MPR_LIST_STABLE)) == 0) {
        return 0;
    }
    for (c = 0; c < schema->nfields; c++) {
        mprAddItem(result, schema->fields[c].name);
    }
    return result;
}



static int sdbGetColumnSchema(Edi *edi, cchar *tableName, cchar *columnName, int *type, int *flags, int *cid)
{
    EdiRec      *schema;
    EdiField    *fp;
    int         c;

    assert(edi);
    assert(tableName && *tableName);
    assert(columnName && *columnName);

    schema = getSchema(edi, tableName);
    for (c = 0; c < schema->nfields; c++) {
        fp = &schema->fields[c];
        if (smatch(columnName, fp->name)) {
            if (type) {
                *type = fp->type;
            }
            if (flags) {
                *flags = fp->flags;
            }
            if (cid) {
                *cid = c;
            }
        }
    }
    return 0;
}


static MprList *sdbGetTables(Edi *edi)
{
    EdiGrid     *grid;
    EdiRec      *rec;
    MprList     *result;
    int         r;

    assert(edi);

    if ((grid = query(edi, "SELECT name from sqlite_master WHERE type = 'table' order by NAME;", NULL)) == 0) {
        return 0;
    }
    if ((result = mprCreateList(0, MPR_LIST_STABLE)) == 0) {
        return 0;
    }
    for (r = 0; r < grid->nrecords; r++) {
        rec = grid->records[r];
        if (sstarts(rec->tableName, "sqlite_")) {
            continue;
        }
        mprAddItem(result, rec->tableName);
    }
    return result;
}


static int sdbGetTableDimensions(Edi *edi, cchar *tableName, int *numRows, int *numCols)
{
    EdiGrid     *grid;
    EdiRec      *schema;

    assert(edi);
    assert(tableName && *tableName);

    if (numRows) {
        *numRows = 0;
    }
    if (numCols) {
        *numCols = 0;
    }
    if (!validName(tableName)) {
        return MPR_ERR_BAD_ARGS;
    }
    if (numRows) {
        if ((grid = query(edi, sfmt("SELECT COUNT(*) FROM %s;", tableName), NULL)) == 0) { 
            return MPR_ERR_BAD_STATE;
        }
        *numRows = grid->nrecords;
    }
    if (numCols) {
        if ((schema = getSchema(edi, tableName)) == 0) {
            return MPR_ERR_CANT_FIND;
        }
        *numCols = schema->nfields;
    }
    return 0;
}


static int sdbLookupField(Edi *edi, cchar *tableName, cchar *fieldName)
{
    EdiRec      *schema;
    int         c;

    assert(edi);
    assert(tableName && *tableName);
    assert(fieldName && *fieldName);
    
    if ((schema = getSchema(edi, tableName)) == 0) {
        return 0;
    }
    for (c = 0; c < schema->nfields; c++) {
        /* Name is second field */
        if (smatch(fieldName, schema->fields[c].name)) {
            return c;
        }
    }
    return MPR_ERR_CANT_FIND;
}


PUBLIC EdiGrid *sdbQuery(Edi *edi, cchar *cmd, int argc, cchar **argv, va_list vargs)
{
    return queryv(edi, cmd, argc, argv, vargs);
}


static EdiField sdbReadField(Edi *edi, cchar *tableName, cchar *key, cchar *fieldName)
{
    EdiField    err;
    EdiGrid     *grid;

    if (!validName(tableName) || !validName(fieldName)) {
        err.valid = 0;
        return err;
    }
    if ((grid = query(edi, sfmt("SELECT %s FROM %s WHERE 'id' = ?;", fieldName, tableName), key, NULL)) == 0) {
        err.valid = 0;
        return err;
    }
    return grid->records[0]->fields[0];
}


static EdiRec *sdbReadRec(Edi *edi, cchar *tableName, cchar *key)
{
    EdiGrid     *grid;

    if (!validName(tableName)) {
        return 0;
    }
    if ((grid = query(edi, sfmt("SELECT * FROM %s WHERE id = ?;", tableName), key, NULL)) == 0) {
        return 0;
    }
    return grid->records[0];
}


static EdiGrid *setTableName(EdiGrid *grid, cchar *tableName)
{
    if (grid && !grid->tableName) {
        grid->tableName = sclone(tableName);
    }
    return grid;
}


static EdiGrid *sdbReadWhere(Edi *edi, cchar *tableName, cchar *columnName, cchar *operation, cchar *value)
{
    EdiGrid     *grid;
    
    assert(tableName && *tableName);

    if (!validName(tableName)) {
        return 0;
    }
    if (columnName) {
        if (!validName(columnName)) {
            return 0;
        }
        assert(columnName && *columnName);
        assert(operation && *operation);
        assert(value);
        grid = query(edi, sfmt("SELECT * FROM %s WHERE %s %s ?;", tableName, columnName, operation), value, NULL);
    } else {
        grid = query(edi, sfmt("SELECT * FROM %s;", tableName), NULL);
    }
    return setTableName(grid, tableName);
}


static int sdbRemoveColumn(Edi *edi, cchar *tableName, cchar *columnName)
{
    mprError("SDB does not support removing columns");
    return MPR_ERR_BAD_STATE;
}


static int sdbRemoveIndex(Edi *edi, cchar *tableName, cchar *indexName)
{
    if (!validName(tableName) || !validName(indexName)) {
        return 0;
    }
    return query(edi, sfmt("DROP INDEX %s;", indexName), NULL) != 0;
}


static int sdbRemoveRec(Edi *edi, cchar *tableName, cchar *key)
{
    assert(edi);
    assert(tableName && *tableName);
    assert(key && *key);

    if (!validName(tableName)) {
        return 0;
    }
    return query(edi, sfmt("DELETE FROM %s WHERE id = ?;", tableName), key, NULL) != 0;
}


static int sdbRemoveTable(Edi *edi, cchar *tableName)
{
    if (!validName(tableName)) {
        return 0;
    }
    return query(edi, sfmt("DROP TABLE IF EXISTS %s;", tableName), NULL) != 0;
}


static int sdbRenameTable(Edi *edi, cchar *tableName, cchar *newTableName)
{
    if (!validName(tableName) || !validName(newTableName)) {
        return 0;
    }
    removeSchema(edi, tableName);
    removeSchema(edi, newTableName);
    return query(edi, sfmt("ALTER TABLE %s RENAME TO %s;", tableName, newTableName), NULL) != 0;
}


static int sdbRenameColumn(Edi *edi, cchar *tableName, cchar *columnName, cchar *newColumnName)
{
    mprError("SQLite does not support renaming columns");
    return MPR_ERR_BAD_STATE;
}


static int sdbSave(Edi *edi)
{
    return 0;
}


/*
    Map values before storing in the database
    While not required, it is prefereable to normalize the storage of some values.
    For example: dates are stored as numbers
 */
static cchar *mapSdbValue(cchar *value, int type)
{
    MprTime     time;

    if (value == 0) {
        return value;
    }
    switch (type) {
    case EDI_TYPE_DATE:
        if (!snumber(value)) {
            mprParseTime(&time, value, MPR_UTC_TIMEZONE, 0);
            value = itos(time);
        }
        break;

    case EDI_TYPE_BINARY:
    case EDI_TYPE_BOOL:
    case EDI_TYPE_FLOAT:
    case EDI_TYPE_INT:
    case EDI_TYPE_STRING:
    case EDI_TYPE_TEXT:
    default:
        break;
    }
    return value;
}


static int sdbUpdateField(Edi *edi, cchar *tableName, cchar *key, cchar *fieldName, cchar *value)
{
    int     type;

    if (!validName(tableName) || !validName(fieldName)) {
        return 0;
    }
    sdbGetColumnSchema(edi, tableName, fieldName, &type, 0, 0);
    value = mapSdbValue(value, type);
    return query(edi, sfmt("UPDATE %s SET %s TO ? WHERE 'id' = ?;", tableName, fieldName), value, key, NULL) != 0;
}


/*
    Use parameterized queries to reduce the risk of SQL injection
 */
static int sdbUpdateRec(Edi *edi, EdiRec *rec)
{
    MprBuf      *buf;
    EdiField    *fp;
    cchar       **argv;
    int         argc, f;

    if (!ediValidateRec(rec)) {
        return MPR_ERR_CANT_WRITE;
    }
    if ((argv = mprAlloc(((rec->nfields * 2) + 2) * sizeof(cchar*))) == 0) {
        return MPR_ERR_MEMORY;
    }
    argc = 0;

    buf = mprCreateBuf(0, 0);
    if (rec->id) {
        mprPutToBuf(buf, "UPDATE %s SET ", rec->tableName);
        for (f = 0; f < rec->nfields; f++) {
            fp = &rec->fields[f];
            mprPutToBuf(buf, "%s = ?, ", fp->name);
            argv[argc++] = mapSdbValue(fp->value, fp->type);
        }
        mprAdjustBufEnd(buf, -2);
        mprPutStringToBuf(buf, " WHERE id = ?;");
        argv[argc++] = rec->id;

    } else {
        mprPutToBuf(buf, "INSERT INTO %s (", rec->tableName);
        for (f = 1; f < rec->nfields; f++) {
            fp = &rec->fields[f];
            mprPutToBuf(buf, "%s,", fp->name);
        }
        mprAdjustBufEnd(buf, -1);
        mprPutStringToBuf(buf, ") VALUES (");
        for (f = 1; f < rec->nfields; f++) {
            mprPutStringToBuf(buf, "?,");
            fp = &rec->fields[f];
            argv[argc++] = mapSdbValue(fp->value, fp->type);
        }
        mprAdjustBufEnd(buf, -1);
        mprPutCharToBuf(buf, ')');
    }
    argv[argc] = NULL;

    if (queryArgv(edi, mprGetBufStart(buf), argc, argv) == 0) {
        return MPR_ERR_CANT_WRITE;
    }
    return 0;
}


PUBLIC cchar *sdbGetLastError(Edi *edi)
{
    return ((Sdb*) edi)->edi.errMsg;
}


/*********************************** Support *******************************/

static EdiGrid *query(Edi *edi, cchar *cmd, ...)
{
    EdiGrid     *grid;
    va_list     args;

    va_start(args, cmd);
    grid = queryv(edi, cmd, 0, NULL, args);
    va_end(args);
    return grid;
}


/*
    Vars are ignored. Just to satisify old compilers
 */
static EdiGrid *queryArgv(Edi *edi, cchar *cmd, int argc, cchar **argv, ...)
{
    va_list     vargs;
    EdiGrid     *grid;

    va_start(vargs, argv);
    grid = queryv(edi, cmd, argc, argv, vargs);
    va_end(vargs);
    return grid;
}


/*
    This function supports parameterized queries. Provide parameters for query via either argc+argv or vargs.
    Strongly recommended to use parameterized queries to lessen SQL injection risk.
 */
static EdiGrid *queryv(Edi *edi, cchar *cmd, int argc, cchar **argv, va_list vargs)
{
    Sdb             *sdb;
    sqlite3         *db;
    sqlite3_stmt    *stmt;
    EdiGrid         *grid;
    EdiRec          *rec, *schema;
    MprList         *result;
    char            *tableName;
    cchar           *tail, *colName, *value, *defaultTableName, *arg;
    ssize           len;
    int             r, nrows, i, ncol, rc, retries, index, type;

    assert(edi);
    assert(cmd && *cmd);

    sdb = (Sdb*) edi;
    retries = 0;
    sdb->edi.errMsg = 0;

    if ((db = sdb->db) == 0) {
        sdbError(edi, "Database '%s' is closed", sdb->edi.path);
        return 0;
    }
    if ((result = mprCreateList(0, MPR_LIST_STABLE)) == 0) {
        return 0;
    }
    defaultTableName = 0;
    rc = SQLITE_OK;
    nrows = 0;

    while (cmd && *cmd && (rc == SQLITE_OK || (rc == SQLITE_SCHEMA && ++retries < 2))) {
        stmt = 0;
        mprLog(2, "SQL: %s", cmd);
        rc = sqlite3_prepare_v2(db, cmd, -1, &stmt, &tail);
        if (rc != SQLITE_OK) {
            sdbTrace(edi, 2, "SDB: cannot prepare command: %s, error: %s", cmd, sqlite3_errmsg(db));
            continue;
        }
        if (stmt == 0) {
            /* Comment or white space */
            cmd = tail;
            continue;
        }
        if (argc == 0) {
            for (index = 0; (arg = va_arg(vargs, cchar*)) != 0; index++) {
                if (sqlite3_bind_text(stmt, index + 1, arg, -1, 0) != SQLITE_OK) {
                    sdbError(edi, "SDB: cannot bind to arg: %d, %s, error: %s", index + 1, arg, sqlite3_errmsg(db));
                    return 0;
                }
            }
        } else if (argv) {
            for (index = 0; index < argc; index++) {
                if (sqlite3_bind_text(stmt, index + 1, argv[index], -1, 0) != SQLITE_OK) {
                    sdbError(edi, "SDB: cannot bind to arg: %d, %s, error: %s", index + 1, argv[index], sqlite3_errmsg(db));
                    return 0;
                }
            } 
        }
        ncol = sqlite3_column_count(stmt);
        for (nrows = 0; ; nrows++) {
            if (sqlite3_step(stmt) == SQLITE_ROW) {
                tableName = (char*) sqlite3_column_table_name(stmt, 0);
                if ((rec = createBareRec(edi, tableName, ncol)) == 0) {
                    sqlite3_finalize(stmt);
                    return 0;
                }
                if (defaultTableName == 0) {
                    defaultTableName = rec->tableName;
                }
                mprAddItem(result, rec);
                for (i = 0; i < ncol; i++) {
                    colName = sqlite3_column_name(stmt, i);
                    value = (cchar*) sqlite3_column_text(stmt, i);
                    if (tableName && strcmp(tableName, defaultTableName) != 0) {
                        len = strlen(tableName) + 1;
                        tableName = sjoin("_", tableName, colName, NULL);
                        tableName[len] = toupper((uchar) tableName[len]);
                    }
                    if (tableName && ((schema = getSchema(edi, tableName)) != 0)) {
                        rec->fields[i] = makeRecField(value, colName, schema->fields[i].type);
                    } else {
                        type = sqlite3_column_type(stmt, i);
                        rec->fields[i] = makeRecField(value, colName, mapSqliteTypeToEdiType(type));
                    }
                    if (smatch(colName, "id")) {
                        rec->fields[i].flags |= EDI_KEY;
                        rec->id = rec->fields[i].value;
                    }
                }
            } else {
                rc = sqlite3_finalize(stmt);
                stmt = 0;
                if (rc != SQLITE_SCHEMA) {
                    retries = 0;
                    for (cmd = tail; isspace((uchar) *cmd); cmd++) {}
                }
                break;
            }
        }
    }
    if (stmt) {
        rc = sqlite3_finalize(stmt);
    }
    if (rc != SQLITE_OK) {
        if (rc == sqlite3_errcode(db)) {
            sdbTrace(edi, 2, "SDB: cannot run query: %s, error: %s", cmd, sqlite3_errmsg(db));
        } else {
            sdbTrace(edi, 2, "SDB: unspecified SQL error for: %s", cmd);
        }
        return 0;
    }
    if ((grid = ediCreateBareGrid(edi, defaultTableName, nrows)) == 0) {
        return 0;
    }
    for (r = 0; r < nrows; r++) {
        grid->records[r] = mprGetItem(result, r);
    }
    return grid;
}


static EdiRec *createBareRec(Edi *edi, cchar *tableName, int nfields)
{
    EdiRec  *rec;

    if ((rec = mprAllocMem(sizeof(EdiRec) + sizeof(EdiField) * nfields, MPR_ALLOC_MANAGER | MPR_ALLOC_ZERO)) == 0) {
        return 0;
    }
    mprSetManager(rec, (MprManager) ediManageEdiRec);
    rec->edi = edi;
    rec->tableName = sclone(tableName);
    rec->nfields = nfields;
    return rec;
}


static EdiField makeRecField(cchar *value, cchar *name, int type)
{
    EdiField    f;

    f.valid = 1;
    f.value = sclone(value);
    f.name = sclone(name);
    f.type = type;
    f.flags = 0;
    return f;
}


static void *allocBlock(int size)
{
    void    *ptr;

    if ((ptr = mprAlloc(size)) != 0) {
        mprHold(ptr);
    }
    return ptr;
}


static void freeBlock(void *ptr)
{
    mprRelease(ptr);
}


static void *reallocBlock(void *ptr, int size)
{
    mprRelease(ptr);
    if ((ptr =  mprRealloc(ptr, size)) != 0) {
        mprHold(ptr);
    }
    return ptr;
}


static int blockSize(void *ptr)
{
    return (int) mprGetBlockSize(ptr);
}


static int roundBlockSize(int size)
{
    return MPR_ALLOC_ALIGN(size);
}


static int initAllocator(void *data)
{
    return 0;
}


static void termAllocator(void *data)
{
}


struct sqlite3_mem_methods mem = {
    allocBlock, freeBlock, reallocBlock, blockSize, roundBlockSize, initAllocator, termAllocator, NULL 
};


static cchar *mapToSqlType(int type)
{
    assert(0 < type && type < EDI_TYPE_MAX);
    return dataTypeToSqlType[type];
}


static int mapToEdiType(cchar *type)
{
    int     i;

    for (i = 0; i < EDI_TYPE_MAX; i++) {
        if (smatch(dataTypeToSqlType[i], type)) {
            return i;
        }
    }
    mprError("SDB: Cannot find type %s", type);
    assert(0);
    return 0;
}


static int mapSqliteTypeToEdiType(int type) 
{
    if (type == SQLITE_INTEGER) {
        return EDI_TYPE_INT;
    } else if (type == SQLITE_FLOAT) {
        return EDI_TYPE_FLOAT;
    } else if (type == SQLITE_TEXT) {
        return EDI_TYPE_TEXT;
    } else if (type == SQLITE_BLOB) {
        return EDI_TYPE_BINARY;
    } else if (type == SQLITE_NULL) {
        return EDI_TYPE_TEXT;
    }
    mprError("SDB: Cannot find query type %d", type);
    assert(0);
    return 0;
}


static bool validName(cchar *str)
{
    cchar   *cp;

    if (!str) {
        return 0;
    }
    if (!isalpha(*str) && *str != '_') {
        return 0;
    }
    for (cp = &str[1]; *cp && (isalnum((uchar) *cp) || *cp == '_' || *cp == '$'); cp++) {}
    return (*cp == '\0');
}


static void sdbError(Edi *edi, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    edi->errMsg = sfmtv(fmt, args);
    va_end(args);
    mprError("%s", edi->errMsg);
}


static void sdbTrace(Edi *edi, int level, cchar *fmt, ...)
{
    va_list     args;

    va_start(args, fmt);
    edi->errMsg = sfmtv(fmt, args);
    va_end(args);
    mprTrace(level, "%s", edi->errMsg);
}


/*********************************** Factory *******************************/

static void initSqlite()
{
    mprGlobalLock();
    if (!sqliteInitialized) {
        sqlite3_config(SQLITE_CONFIG_MALLOC, &mem);
        sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
        if (sqlite3_initialize() != SQLITE_OK) {
            mprError("Cannot initialize SQLite");
            return;
        }
        sqliteInitialized = 1;
    }
    mprGlobalUnlock();
}

#else
/* To prevent ar/ranlib warnings */
PUBLIC void sdbDummy() {}
#endif /* ME_COM_ESP && ME_COM_SQLITE */
#endif /* ME_COM_ESP */

/*
    @copy   default

    Copyright (c) Embedthis Software. All Rights Reserved.

    This software is distributed under commercial and open source licenses.
    You may use the Embedthis Open Source license or you may acquire a 
    commercial license from Embedthis Software. You agree to be fully bound
    by the terms of either license. Consult the LICENSE.md distributed with
    this software for full details and other copyrights.

    Local variables:
    tab-width: 4
    c-basic-offset: 4
    End:
    vim: sw=4 ts=4 expandtab

    @end
 */
