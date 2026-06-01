/** @file
  Shell helpers for UiSetup-backed config access.

  Copyright (c) 2025, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "Include/UiCommon.h"
#include "ConfigBridge.h"
#include "FormEngine.h"
#include "UiDescData.h"

#include <Library/ConsoleOutLib.h>
#include <Library/PcdLib.h>

// UiCommon.h already pulls PrintLib; map ShellPrint to the same console sink.
#define ShellPrint      ConsolePrintUnicode


#define UI_SHELL_MAX_VALUE_CHARS  128

STATIC
EFI_STATUS
UiShellInit (
  VOID
  )
{
  EFI_STATUS  Status;

  Status = FormInit (mUiDescBin, sizeof (mUiDescBin));
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return CfgLoad ();
}

STATIC
CHAR8 *
UnicodeToAsciiAlloc (
  IN CONST CHAR16  *String
  )
{
  CHAR8   *Ascii;
  UINTN    Size;

  if (String == NULL) {
    return NULL;
  }

  Size = StrLen (String) + 1;
  Ascii = AllocateZeroPool (Size);
  if (Ascii == NULL) {
    return NULL;
  }

  if (EFI_ERROR (UnicodeStrToAsciiStrS (String, Ascii, Size))) {
    FreePool (Ascii);
    return NULL;
  }

  return Ascii;
}

STATIC
CHAR8
AsciiToLowerChar (
  IN CHAR8  Value
  )
{
  if ((Value >= 'A') && (Value <= 'Z')) {
    return (CHAR8)(Value - 'A' + 'a');
  }
  return Value;
}

STATIC
BOOLEAN
AsciiCharEqualInsensitive (
  IN CHAR8  Left,
  IN CHAR8  Right
  )
{
  return (BOOLEAN)(AsciiToLowerChar (Left) == AsciiToLowerChar (Right));
}

STATIC
BOOLEAN
AsciiContainsInsensitive (
  IN CONST CHAR8  *String,
  IN CONST CHAR8  *Pattern
  )
{
  UINTN  Index;
  UINTN  Match;

  if ((String == NULL) || (Pattern == NULL)) {
    return FALSE;
  }

  if (*Pattern == '\0') {
    return TRUE;
  }

  for (Index = 0; String[Index] != '\0'; Index++) {
    for (Match = 0; Pattern[Match] != '\0'; Match++) {
      if ((String[Index + Match] == '\0') || !AsciiCharEqualInsensitive (String[Index + Match], Pattern[Match])) {
        break;
      }
    }
    if (Pattern[Match] == '\0') {
      return TRUE;
    }
  }

  return FALSE;
}

STATIC
EFI_STATUS
ParseNumericValue (
  IN  CONST CHAR16  *ValueString,
  OUT UINT32        *Value
  )
{
  EFI_STATUS    Status;
  UINTN         Parsed;
  CHAR16       *End;
  CONST CHAR16 *Walker;

  if ((ValueString == NULL) || (Value == NULL) || (*ValueString == L'\0')) {
    return EFI_INVALID_PARAMETER;
  }

  Walker = ValueString;
  if ((Walker[0] == L'0') && ((Walker[1] == L'x') || (Walker[1] == L'X'))) {
    Status = StrHexToUintnS (Walker, &End, &Parsed);
  } else {
    Status = StrDecimalToUintnS (Walker, &End, &Parsed);
  }

  if (EFI_ERROR (Status) || (*End != L'\0') || (Parsed > MAX_UINT32)) {
    return EFI_INVALID_PARAMETER;
  }

  *Value = (UINT32)Parsed;
  return EFI_SUCCESS;
}

STATIC
BOOLEAN
IsFieldByteAligned (
  IN UI_FIELD_ENTRY  *Field
  )
{
  return (BOOLEAN)((Field->BitOffset % 8 == 0) && (Field->BitLength % 8 == 0));
}

STATIC
UINT32
PackBytesToUint32 (
  IN CONST UINT8  *Bytes,
  IN UINTN         ByteCount
  )
{
  UINT32  Value;
  UINTN   Index;

  Value = 0;
  for (Index = 0; Index < ByteCount && Index < sizeof (UINT32); Index++) {
    Value |= ((UINT32)Bytes[Index]) << (Index * 8);
  }

  return Value;
}

STATIC
EFI_STATUS
ApplyTextValue (
  IN  UI_FIELD_ENTRY  *Field,
  IN  UINT8           *TagData,
  IN  CONST CHAR16    *ValueString,
  OUT UINT32          *PackedValue
  )
{
  CHAR8  *AsciiValue;
  UINT8  *FieldBytes;
  UINTN   ByteCount;
  UINTN   Length;

  if ((Field == NULL) || (TagData == NULL) || (ValueString == NULL) || (PackedValue == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  if ((Field->BitLength > 32) || !IsFieldByteAligned (Field)) {
    return EFI_UNSUPPORTED;
  }

  ByteCount = Field->BitLength / 8;
  AsciiValue = UnicodeToAsciiAlloc (ValueString);
  if (AsciiValue == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Length = AsciiStrLen (AsciiValue);
  if (Length > ByteCount) {
    FreePool (AsciiValue);
    return EFI_BAD_BUFFER_SIZE;
  }

  FieldBytes = TagData + (Field->BitOffset / 8);
  ZeroMem (FieldBytes, ByteCount);
  if (Length > 0) {
    CopyMem (FieldBytes, AsciiValue, Length);
  }

  *PackedValue = PackBytesToUint32 (FieldBytes, ByteCount);
  FreePool (AsciiValue);
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
ResolveFieldValue (
  IN  UI_FIELD_ENTRY  *Field,
  IN  CONST CHAR16    *ValueString,
  IN  UINT8           *TagData,
  OUT UINT32          *Value,
  OUT BOOLEAN         *UsedDirectWrite
  )
{
  EFI_STATUS       Status;
  CHAR8           *AsciiValue;
  UI_OPTION_ENTRY *Options;
  UINT16           Count;
  UINT16           Index;
  UINT32           MaxValue;

  if ((Field == NULL) || (ValueString == NULL) || (TagData == NULL) ||
      (Value == NULL) || (UsedDirectWrite == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *UsedDirectWrite = FALSE;

  if (Field->FieldType == UI_FIELD_TYPE_EDITTEXT) {
    *UsedDirectWrite = TRUE;
    return ApplyTextValue (Field, TagData, ValueString, Value);
  }

  AsciiValue = NULL;

  Status = ParseNumericValue (ValueString, Value);
  if ((Field->FieldType == UI_FIELD_TYPE_COMBO) && EFI_ERROR (Status)) {
    AsciiValue = UnicodeToAsciiAlloc (ValueString);
    if (AsciiValue == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    Options = FormGetOptions (Field, &Count);
    for (Index = 0; Index < Count; Index++) {
      if (AsciiStriCmp (FormGetString (Options[Index].LabelStrOffset), AsciiValue) == 0) {
        *Value = Options[Index].Value;
        FreePool (AsciiValue);
        return EFI_SUCCESS;
      }
    }
    FreePool (AsciiValue);
    return EFI_INVALID_PARAMETER;
  }

  if (EFI_ERROR (Status)) {
    return Status;
  }

  if ((Field->FieldType == UI_FIELD_TYPE_COMBO) && (Field->OptionCount > 0)) {
    Options = FormGetOptions (Field, &Count);
    for (Index = 0; Index < Count; Index++) {
      if (Options[Index].Value == *Value) {
        return EFI_SUCCESS;
      }
    }
    return EFI_INVALID_PARAMETER;
  }

  MaxValue = (Field->BitLength >= 32) ? MAX_UINT32 : ((1U << Field->BitLength) - 1U);
  if (*Value > MaxValue) {
    return EFI_INVALID_PARAMETER;
  }

  return EFI_SUCCESS;
}

STATIC
VOID
PrintFieldValue (
  IN UI_FIELD_ENTRY  *Field,
  IN UINT8           *TagData
  )
{
  UI_OPTION_ENTRY *Options;
  UINT16           Count;
  UINT16           Index;
  UINT32           Value;
  UINT8           *FieldBytes;
  UINTN            ByteCount;
  CHAR8            ValueBuffer[UI_SHELL_MAX_VALUE_CHARS];
  UINTN            CopyLength;

  if ((Field == NULL) || (TagData == NULL)) {
    return;
  }

  if (Field->FieldType == UI_FIELD_TYPE_EDITTEXT) {
    if (!IsFieldByteAligned (Field)) {
      ShellPrint (L"<unsupported text layout>");
      return;
    }

    ByteCount = Field->BitLength / 8;
    if (ByteCount >= sizeof (ValueBuffer)) {
      ByteCount = sizeof (ValueBuffer) - 1;
    }
    FieldBytes = TagData + (Field->BitOffset / 8);
    ZeroMem (ValueBuffer, sizeof (ValueBuffer));
    CopyLength = ByteCount;
    CopyMem (ValueBuffer, FieldBytes, CopyLength);
    ValueBuffer[sizeof (ValueBuffer) - 1] = '\0';
    ShellPrint (L"\"%a\"", ValueBuffer);
    return;
  }

  Value = FormGetFieldValue (Field, TagData);
  if (Field->FieldType == UI_FIELD_TYPE_COMBO) {
    Options = FormGetOptions (Field, &Count);
    for (Index = 0; Index < Count; Index++) {
      if (Options[Index].Value == Value) {
        ShellPrint (L"%u (0x%x, %a)", Value, Value, FormGetString (Options[Index].LabelStrOffset));
        return;
      }
    }
  }

  ShellPrint (L"%u (0x%x)", Value, Value);
}

STATIC
VOID
PrintUsageSet (
  IN CONST CHAR16  *Command
  )
{
  ShellPrint (L"Usage: %s <field.path> <value>\n", Command);
  ShellPrint (L"       Value accepts decimal, hex, combo labels, or quoted text for short EditText fields\n");
}

STATIC
VOID
PrintUsageGet (
  IN CONST CHAR16  *Command
  )
{
  ShellPrint (L"Usage: %s <field.path>\n", Command);
}

STATIC
VOID
PrintUsageFind (
  IN CONST CHAR16  *Command
  )
{
  ShellPrint (L"Usage: %s [pattern] [--verbose]\n", Command);
  ShellPrint (L"       --verbose or -v: show field display names in brackets\n");
}

EFI_STATUS
EFIAPI
UiCfgShellCfgSetCommand (
  IN UINTN    Argc,
  IN CHAR16  *Argv[]
  )
{
  EFI_STATUS      Status;
  CHAR8          *FieldPath;
  UI_FIELD_ENTRY *Field;
  UINT8          *TagData;
  UINT32          Value;
  BOOLEAN         UsedDirectWrite;

  if (Argc != 3) {
    PrintUsageSet (Argv[0]);
    return EFI_INVALID_PARAMETER;
  }

  Status = UiShellInit ();
  if (EFI_ERROR (Status)) {
    ShellPrint (L"Error: failed to initialize config access: %r\n", Status);
    return Status;
  }

  FieldPath = UnicodeToAsciiAlloc (Argv[1]);
  if (FieldPath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Field = FormFindFieldByPath (FieldPath);
  if (Field == NULL) {
    ShellPrint (L"Error: field '%s' not found\n", Argv[1]);
    FreePool (FieldPath);
    return EFI_NOT_FOUND;
  }

  TagData = CfgGetTagData (Field->TagId);
  if (TagData == NULL) {
    ShellPrint (L"Error: tag 0x%x data not found for '%s'\n", Field->TagId, Argv[1]);
    FreePool (FieldPath);
    return EFI_NOT_FOUND;
  }

  Status = ResolveFieldValue (Field, Argv[2], TagData, &Value, &UsedDirectWrite);
  if (EFI_ERROR (Status)) {
    if (Status == EFI_BAD_BUFFER_SIZE) {
      ShellPrint (L"Error: text value is too long for '%s'\n", Argv[1]);
    } else if (Status == EFI_UNSUPPORTED) {
      ShellPrint (L"Error: '%s' uses a field format this shell path does not support yet\n", Argv[1]);
    } else {
      ShellPrint (L"Error: invalid value '%s' for '%s'\n", Argv[2], Argv[1]);
    }
    FreePool (FieldPath);
    return Status;
  }

  if (!UsedDirectWrite) {
    FormSetFieldValue (Field, TagData, Value);
  }
  CfgRecordFieldChange (Field, Value);
  CfgSetDirty ();
  Status = CfgSave ();
  if (EFI_ERROR (Status)) {
    ShellPrint (L"Error: failed to save CfgDelta: %r\n", Status);
    FreePool (FieldPath);
    return Status;
  }

  ShellPrint (L"%s = ", Argv[1]);
  PrintFieldValue (Field, TagData);
  ShellPrint (L"\n");

  FreePool (FieldPath);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UiCfgShellCfgGetCommand (
  IN UINTN    Argc,
  IN CHAR16  *Argv[]
  )
{
  EFI_STATUS      Status;
  CHAR8          *FieldPath;
  UI_FIELD_ENTRY *Field;
  UINT8          *TagData;

  if (Argc != 2) {
    PrintUsageGet (Argv[0]);
    return EFI_INVALID_PARAMETER;
  }

  Status = UiShellInit ();
  if (EFI_ERROR (Status)) {
    ShellPrint (L"Error: failed to initialize config access: %r\n", Status);
    return Status;
  }

  FieldPath = UnicodeToAsciiAlloc (Argv[1]);
  if (FieldPath == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Field = FormFindFieldByPath (FieldPath);
  if (Field == NULL) {
    ShellPrint (L"Error: field '%s' not found\n", Argv[1]);
    FreePool (FieldPath);
    return EFI_NOT_FOUND;
  }

  TagData = CfgGetTagData (Field->TagId);
  if (TagData == NULL) {
    ShellPrint (L"Error: tag 0x%x data not found for '%s'\n", Field->TagId, Argv[1]);
    FreePool (FieldPath);
    return EFI_NOT_FOUND;
  }

  ShellPrint (L"%s = ", Argv[1]);
  PrintFieldValue (Field, TagData);
  ShellPrint (L"\n");

  FreePool (FieldPath);
  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
UiCfgShellCfgFindCommand (
  IN UINTN    Argc,
  IN CHAR16  *Argv[]
  )
{
  EFI_STATUS      Status;
  CHAR8          *Pattern;
  UINT16          FieldCount;
  UINT16          FieldIdx;
  UINTN           Matches;
  UI_FIELD_ENTRY *Field;
  UINT8          *TagData;
  CHAR8          *Name;
  CHAR8          *Path;
  BOOLEAN         Verbose;
  UINTN           ArgIdx;

  if (Argc > 3) {
    PrintUsageFind (Argv[0]);
    return EFI_INVALID_PARAMETER;
  }

  Pattern = NULL;
  Verbose = FALSE;

  // Parse arguments: pattern and/or --verbose flag
  for (ArgIdx = 1; ArgIdx < Argc; ArgIdx++) {
    if ((StrCmp (Argv[ArgIdx], L"--verbose") == 0) ||
        (StrCmp (Argv[ArgIdx], L"-v") == 0)) {
      Verbose = TRUE;
    } else if (Pattern == NULL) {
      Pattern = UnicodeToAsciiAlloc (Argv[ArgIdx]);
      if (Pattern == NULL) {
        return EFI_OUT_OF_RESOURCES;
      }
    }
  }

  Status = UiShellInit ();
  if (EFI_ERROR (Status)) {
    ShellPrint (L"Error: failed to initialize config access: %r\n", Status);
    if (Pattern != NULL) {
      FreePool (Pattern);
    }
    return Status;
  }

  Matches = 0;
  FieldCount = FormGetFieldCount ();
  for (FieldIdx = 0; FieldIdx < FieldCount; FieldIdx++) {
    Field = FormGetField (FieldIdx);
    if (Field == NULL) {
      continue;
    }

    Name = FormGetString (Field->NameStrOffset);
    Path = FormGetFieldPath (Field);
    if ((Pattern != NULL) &&
        !AsciiContainsInsensitive (Path, Pattern) &&
        !AsciiContainsInsensitive (Name, Pattern)) {
      continue;
    }

    TagData = CfgGetTagData (Field->TagId);
    if (TagData == NULL) {
      continue;
    }

    ShellPrint (L"%a = ", Path);
    PrintFieldValue (Field, TagData);
    if (Verbose) {
      ShellPrint (L"  [%a]\n", Name);
    } else {
      ShellPrint (L"\n");
    }
    Matches++;
  }

  ShellPrint (L"%u match(es)\n", (UINT32)Matches);

  if (Pattern != NULL) {
    FreePool (Pattern);
  }

  return EFI_SUCCESS;
}

