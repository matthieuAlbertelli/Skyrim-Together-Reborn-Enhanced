{
  Typed authoring operation for issue #94. Official xEdit SSE only.
  Invoke-StreModding prepares the private Data, marker and immutable inputs.
  No production Data writes, arbitrary arguments, overrides or human attestation.
  Definitions: TES5Edit/dev-4.1.5 Core/wbDefinitionsTES5.pas.
}
unit STRE_CreateValenHelgenDialogue;

var
  target, narrative, dialogue, branch, news, acceptTopic, newsInfo, acceptInfo: IInterface;
  valen: IInterface;
  root, qfName, tifName: string;
  output: TStringList;

procedure Require(ok: boolean; messageText: string);
begin
  if not ok then raise Exception.Create(messageText);
end;

function FindRecord(sig, edid: string): IInterface;
begin
  Result := MainRecordByEditorID(GroupBySignature(target, sig), edid);
end;

function NewRecord(sig, edid: string): IInterface;
begin
  Require(not Assigned(FindRecord(sig, edid)), 'Already exists: ' + edid);
  if not HasGroup(target, sig) then Add(target, sig, True);
  Result := Add(GroupBySignature(target, sig), sig, True);
  SetElementEditValues(Result, 'EDID', edid);
end;

function AppendEntry(e: IInterface; path: string): IInterface;
var a: IInterface;
begin
  AddMessage('Appending ' + path + ' to ' + FullPath(e));
  if not ElementExists(e, path) then begin
    Add(e, path, True);
    a := ElementByPath(e, path);
    if ElementCount(a) = 0 then ElementAssign(a, HighInteger, nil, False);
  end else begin
    a := ElementByPath(e, path);
    ElementAssign(a, HighInteger, nil, False);
  end;
  Result := ElementByIndex(a, ElementCount(a) - 1);
  Require(Assigned(Result), 'Missing array element: ' + path);
end;

procedure Link(e: IInterface; path: string; linked: IInterface);
begin
  Add(e, path, True);
  SetElementNativeValues(e, path, GetLoadOrderFormID(linked));
  Require(Equals(LinksTo(ElementByPath(e, path)), linked), 'Invalid link: ' + path);
end;

procedure Condition(e: IInterface; func, paramName: string; ref: IInterface;
  compareType: integer; value: float);
var c: IInterface;
begin
  c := AppendEntry(e, 'Conditions');
  SetElementNativeValues(c, 'CTDA\Type', compareType);
  SetElementEditValues(c, 'CTDA\Function', func);
  SetElementNativeValues(c, 'CTDA\Comparison Value - Float', value);
  SetElementNativeValues(c, 'CTDA\' + paramName, GetLoadOrderFormID(ref));
  SetElementEditValues(c, 'CTDA\Run On', 'Subject');
end;

function Topic(edid, text: string): IInterface;
begin
  Result := NewRecord('DIAL', edid);
  Add(Result, 'FULL', True);
  SetElementEditValues(Result, 'FULL', text);
  SetElementNativeValues(Result, 'PNAM', 50.0);
  Link(Result, 'QNAM', dialogue);
  Link(Result, 'BNAM', branch);
  Add(Result, 'DATA', True);
  SetElementNativeValues(Result, 'DATA\Category', 0);
  SetElementNativeValues(Result, 'DATA\Subtype', 0);
  Add(Result, 'SNAM', True);
  SetElementNativeValues(Result, 'SNAM', $54535543); // CUST signature, little endian
end;

function Info(owner: IInterface; edid: string; flags: integer): IInterface;
begin
  Result := Add(owner, 'INFO', True);
  SetElementEditValues(Result, 'EDID', edid);
  Add(Result, 'ENAM', True);
  SetElementNativeValues(Result, 'ENAM\Flags', flags);
  Link(Result, 'TPIC', owner);
  Add(Result, 'PNAM', True);
  SetElementNativeValues(Result, 'PNAM', 0);
  Condition(Result, 'GetIsID', 'Referenceable Object', valen, 0, 1.0);
  Condition(Result, 'GetStage', 'Quest', narrative, $80, 10.0);
end;

procedure Response(infoRecord: IInterface; number: integer; text: string);
var r, responses: IInterface;
begin
  r := AppendEntry(infoRecord, 'Responses');
  SetElementNativeValues(r, 'TRDT\Response number', number);
  SetElementEditValues(r, 'NAM1', text);
end;

function AttachScript(e: IInterface; scriptName: string): IInterface;
var vm: IInterface;
begin
  vm := Add(e, 'VMAD', True);
  SetElementNativeValues(vm, 'Version', 5);
  SetElementNativeValues(vm, 'Object Format', 2);
  Result := AppendEntry(vm, 'Scripts');
  SetElementEditValues(Result, 'ScriptName', scriptName);
end;

procedure Build;
var stage, entry, obj, scriptEntry, prop, fragments, fragment: IInterface;
begin
  narrative := NewRecord('QUST', 'STRE_QUEST_ValenHelgen');
  Add(narrative, 'FULL', True);
  SetElementEditValues(narrative, 'FULL', 'Les ruines de Helgen');
  SetElementNativeValues(narrative, 'DNAM\Flags', $100);
  SetElementNativeValues(narrative, 'DNAM\Priority', 50);
  SetElementNativeValues(narrative, 'DNAM\Type', 8);
  Add(narrative, 'Stages', True);
  stage := ElementByIndex(ElementByPath(narrative, 'Stages'), 0);
  SetElementNativeValues(stage, 'INDX\Stage Index', 0);
  stage := ElementAssign(ElementByPath(narrative, 'Stages'), HighInteger, nil, False);
  SetElementNativeValues(stage, 'INDX\Stage Index', 10);
  // QUST Stage definition: member 0=INDX, member 1=Log Entries.
  // Add(stage,'QSDT') creates the array but asserts while returning a nested field.
  // Assign the documented schema member directly, never a record's file position.
  ElementAssign(stage, 1, nil, False);
  entry := ElementByIndex(ElementByPath(stage, 'Log Entries'), 0);
  Require(Assigned(entry), 'Stage 10 log entry was not materialized');
  Add(entry, 'CNAM', True);
  SetElementEditValues(entry, 'CNAM', 'Valen nous a parlé d''une attaque de dragon à Helgen. Nous avons choisi d''aller enquêter sur place.');
  Add(narrative, 'Objectives', True);
  obj := ElementByIndex(ElementByPath(narrative, 'Objectives'), 0);
  SetElementNativeValues(obj, 'QOBJ', 10);
  Add(obj, 'FNAM', True);
  SetElementEditValues(obj, 'NNAM', 'Se rendre à Helgen pour enquêter sur l''attaque du dragon.');

  dialogue := NewRecord('QUST', 'STRE_QUEST_ValenDialogue');
  SetElementNativeValues(dialogue, 'DNAM\Flags', $8101); // SGE, Run Once, Has Dialogue Data
  SetElementNativeValues(dialogue, 'DNAM\Priority', 50);
  SetElementNativeValues(dialogue, 'DNAM\Type', 0);
  branch := NewRecord('DLBR', 'STRE_DLBR_ValenHelgen');
  Link(branch, 'QNAM', dialogue);
  Add(branch, 'TNAM', True);
  SetElementNativeValues(branch, 'TNAM', 0);
  Add(branch, 'DNAM', True);
  SetElementNativeValues(branch, 'DNAM', 1);
  news := Topic('STRE_DIAL_ValenHelgen_News', 'Quelles nouvelles avez-vous de Helgen ?');
  acceptTopic := Topic('STRE_DIAL_ValenHelgen_Accept', 'Nous irons peut-être voir ce qui s''y est passé.');
  Link(branch, 'SNAM', news);
  newsInfo := Info(news, 'STRE_INFO_ValenHelgen_News', $A00);
  Response(newsInfo, 1, 'Des nouvelles nous sont parvenues de Helgen. La ville aurait été attaquée par un dragon.');
  Response(newsInfo, 2, 'Les récits sont confus, mais quelque chose de grave s''y est produit.');
  Add(newsInfo, 'Link To', True);
  SetNativeValue(ElementByIndex(ElementByPath(newsInfo, 'Link To'), 0), GetLoadOrderFormID(acceptTopic));
  acceptInfo := Info(acceptTopic, 'STRE_INFO_ValenHelgen_Accept', $A01);
  Response(acceptInfo, 1, 'Faites-le. Mais restez prudents. Si ces récits sont vrais, Helgen pourrait n''être que le commencement.');
  Add(news, 'TIFC', True);
  SetElementNativeValues(news, 'TIFC', 1);
  Add(acceptTopic, 'TIFC', True);
  SetElementNativeValues(acceptTopic, 'TIFC', 1);

  // CK-style names are derived only AFTER xEdit allocates real identities.
  qfName := 'QF_STRE_QUEST_ValenHelgen_' + IntToHex(FixedFormID(narrative), 8);
  tifName := 'TIF_' + IntToHex(FixedFormID(acceptInfo), 8);
  scriptEntry := AttachScript(narrative, qfName);
  ElementAssign(ElementByPath(narrative, 'VMAD'), 3, nil, False);
  ElementAssign(ElementByPath(narrative, 'VMAD'), 4, nil, False);
  fragments := ElementByPath(narrative, 'VMAD\Script Fragments');
  SetElementEditValues(fragments, 'FileName', qfName);
  fragment := AppendEntry(fragments, 'Fragments');
  SetElementNativeValues(fragment, 'Quest Stage', 10);
  SetElementNativeValues(fragment, 'Quest Stage Index', 0);
  SetElementEditValues(fragment, 'ScriptName', qfName);
  SetElementEditValues(fragment, 'FragmentName', 'Fragment_0');

  scriptEntry := AttachScript(acceptInfo, tifName);
  prop := AppendEntry(scriptEntry, 'Properties');
  SetElementEditValues(prop, 'propertyName', 'ValenHelgen');
  SetElementNativeValues(prop, 'Type', 1);
  SetElementNativeValues(prop, 'Flags', 1);
  SetElementNativeValues(prop, 'Value\Object Union\Object v2\Alias', -1);
  SetElementNativeValues(prop, 'Value\Object Union\Object v2\FormID', GetLoadOrderFormID(narrative));
  ElementAssign(ElementByPath(acceptInfo, 'VMAD'), 3, nil, False);
  fragments := ElementByPath(acceptInfo, 'VMAD\Script Fragments');
  SetElementNativeValues(fragments, 'Flags', 1);
  SetElementEditValues(fragments, 'FileName', tifName);
  if ElementCount(ElementByPath(fragments, 'Fragments')) = 0 then
    ElementAssign(ElementByPath(fragments, 'Fragments'), HighInteger, nil, False);
  fragment := ElementByIndex(ElementByPath(fragments, 'Fragments'), 0);
  SetElementEditValues(fragment, 'ScriptName', tifName);
  SetElementEditValues(fragment, 'FragmentName', 'Fragment_0');
end;

procedure ExportPrivate(fileName: string);
var stream: TFileStream;
begin
  Require(not FileExists(root + fileName), 'Output already exists');
  stream := TFileStream.Create(root + fileName, fmCreate);
  try
    FileWriteToStream(target, stream, False);
  finally
    stream.Free;
  end;
end;

function Initialize: integer;
var i: integer; config: TStringList; mode: string;
begin
  Result := 0;
  root := ExtractFileDir(Copy(DataPath, 1, Length(DataPath) - 1)) + '\';
  Require(Pos('\_audit\modding\', LowerCase(root)) > 0, 'Not a private modding session');
  Require(SameText(DataPath, root + 'Data\'), 'Unexpected private Data path');
  Require(FileExists(root + 'authoring-mode.txt'), 'Missing wrapper marker');
  config := TStringList.Create;
  try
    config.LoadFromFile(root + 'authoring-mode.txt');
    Require(config.Count = 2, 'Invalid wrapper marker');
    Require(SameText(config[0], DataPath), 'Marker/Data mismatch');
    mode := config[1];
    Require((mode = 'DryRun') or (mode = 'Prepare'), 'Invalid authoring mode');
  finally
    config.Free;
  end;
  for i := 0 to FileCount - 1 do
    if GetFileName(FileByIndex(i)) = 'STRE_AlternateStart.esp' then target := FileByIndex(i);
  Require(Assigned(target), 'Missing STRE_AlternateStart.esp');
  Require(MasterCount(target) = 3, 'Unexpected masters');
  Require(GetFileName(MasterByIndex(target, 0)) = 'Skyrim.esm', 'Wrong first master');
  Require(GetFileName(MasterByIndex(target, 1)) = 'Update.esm', 'Wrong second master');
  Require(GetFileName(MasterByIndex(target, 2)) = 'Dragonborn.esm', 'Wrong third master');
  valen := FindRecord('NPC_', 'STRE_NPC_MessireValen');
  Require(Assigned(valen), 'Valen NPC missing');
  Require(EditorID(LinksTo(ElementByPath(valen, 'VTCK'))) = 'MaleWarlock', 'Unexpected Valen voice type');
  Require(Assigned(FindRecord('QUST', 'STRE_QUEST_HelgenInvestigation')), 'Technical Helgen missing');
  Require(not Assigned(FindRecord('QUST', 'STRE_QUEST_ValenHelgen')), 'Narrative already exists');
  Require(not Assigned(FindRecord('QUST', 'STRE_QUEST_ValenDialogue')), 'Dialogue owner already exists');
  Require(not HasGroup(target, 'DIAL'), 'Unexpected existing dialogue; review required');
  Require(not HasGroup(target, 'DLBR'), 'Unexpected existing branch; review required');
  output := TStringList.Create;
  try
    if mode = 'Prepare' then begin
      try
        Build;
        ExportPrivate('authored.esp');
      except
        on E: Exception do begin
          // Preserve failure evidence privately. Script mode closes without save.
          output.Add('FAIL: ' + E.Message);
          output.SaveToFile(root + 'authoring-result.txt');
          ExportPrivate('rejected.esp');
          Exit;
        end;
      end;
    end;
    output.Add('SUCCESS ' + mode);
    output.SaveToFile(root + 'authoring-result.txt');
  finally
    output.Free;
  end;
end;
end.
