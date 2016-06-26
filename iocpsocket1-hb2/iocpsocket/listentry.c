#include "stdafx.h"

void InitializeListHead(PLIST_ENTRY ListHead)
{
    ListHead->Flink = ListHead->Blink = ListHead;
}

BOOL IsListEmpty(LIST_ENTRY * ListHead)
{
    return (BOOL)(ListHead->Flink == ListHead);
}

BOOL RemoveEntryList(PLIST_ENTRY Entry)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Flink;
    Flink = Entry->Flink;
    Blink = Entry->Blink;
    Blink->Flink = Flink;
    Flink->Blink = Blink;
    return (BOOL)(Flink == Blink);
}

PLIST_ENTRY RemoveHeadList(PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Flink;
    PLIST_ENTRY Entry;
    Entry = ListHead->Flink;
    Flink = Entry->Flink;
    ListHead->Flink = Flink;
    Flink->Blink = ListHead;
    return Entry;
}

PLIST_ENTRY RemoveTailList(PLIST_ENTRY ListHead)
{
    PLIST_ENTRY Blink;
    PLIST_ENTRY Entry;
    Entry = ListHead->Blink;
    Blink = Entry->Blink;
    ListHead->Blink = Blink;
    Blink->Flink = ListHead;
    return Entry;
}

void InsertTailList(PLIST_ENTRY ListHead,PLIST_ENTRY Entry)
{
    PLIST_ENTRY Blink;
    Blink = ListHead->Blink;
    Entry->Flink = ListHead;
    Entry->Blink = Blink;
    Blink->Flink = Entry;
    ListHead->Blink = Entry;
}

void InsertHeadList(PLIST_ENTRY ListHead,PLIST_ENTRY Entry)
{
    PLIST_ENTRY Flink;
    Flink = ListHead->Flink;
    Entry->Flink = Flink;
    Entry->Blink = ListHead;
    Flink->Blink = Entry;
    ListHead->Flink = Entry;
}
