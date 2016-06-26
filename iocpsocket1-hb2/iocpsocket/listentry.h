
#ifndef LISTENTRY_H
#define	LISTENTRY_H

#ifdef	__cplusplus
extern "C" {
#endif

void InitializeListHead(PLIST_ENTRY ListHead);

BOOL IsListEmpty(LIST_ENTRY * ListHead);

BOOL RemoveEntryList(PLIST_ENTRY Entry);

PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHead);

PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHead);

void InsertTailList(PLIST_ENTRY ListHead,PLIST_ENTRY Entry);

void InsertHeadList(PLIST_ENTRY ListHead,PLIST_ENTRY Entry);

#ifdef	__cplusplus
}
#endif

#endif	/* LISTENTRY_H */

