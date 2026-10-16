{ Presentation-only projection. No new records, links, fragments or assets. }
unit STRE_ValenPresentation;
var target, e: IInterface;
    root, mode: string;
    output: TStringList;

procedure Require(ok: boolean; message: string);
begin
  if not ok then raise Exception.Create(message);
end;

function FindByEditorID(edid: string): IInterface;
var group, topic: IInterface; i: integer;
begin
  Result := MainRecordByEditorID(GroupBySignature(target,'DIAL'),edid);
  if not Assigned(Result) then Result := MainRecordByEditorID(GroupBySignature(target,'QUST'),edid);
  if not Assigned(Result) then Result := MainRecordByEditorID(GroupBySignature(target,'NPC_'),edid);
  if not Assigned(Result) then begin
    group := GroupBySignature(target,'DIAL');
    for i := 0 to ElementCount(group)-1 do begin
      topic := ElementByIndex(group,i);
      if Signature(topic)='DIAL' then Result := MainRecordByEditorID(ChildGroup(topic),edid);
      if Assigned(Result) then Break;
    end;
  end;
  Require(Assigned(Result),'Missing presentation record: '+edid);
end;

procedure Build;
begin
  { GENERATED_PRESENTATION }
  SortMasters(target);
end;

procedure ExportPrivate;
var stream: TFileStream;
begin
  Require(not FileExists(root+'authored.esp'),'Output already exists');
  stream := TFileStream.Create(root+'authored.esp',fmCreate);
  try FileWriteToStream(target,stream,False); finally stream.Free; end;
end;

function Initialize: integer;
var config: TStringList; i: integer;
begin
  Result := 0;
  root := ExtractFileDir(Copy(DataPath,1,Length(DataPath)-1))+'\';
  Require(Pos('\_audit\modding\',LowerCase(root))>0,'Not a private session');
  config := TStringList.Create;
  try
    config.LoadFromFile(root+'authoring-mode.txt');
    Require(config.Count=2,'Invalid wrapper marker');
    Require(SameText(config[0],DataPath),'Marker/Data mismatch');
    mode := config[1];
    Require((mode='DryRun') or (mode='Prepare'),'Invalid mode');
  finally config.Free; end;
  for i := 0 to FileCount-1 do
    if GetFileName(FileByIndex(i))='STRE_AlternateStart.esp' then target := FileByIndex(i);
  Require(Assigned(target),'Missing private plugin');
  Require(MasterCount(target)=3,'Unexpected masters');
  Require(GetFileName(MasterByIndex(target,0))='Skyrim.esm','Master order');
  Require(GetFileName(MasterByIndex(target,1))='Update.esm','Master order');
  Require(GetFileName(MasterByIndex(target,2))='Dragonborn.esm','Master order');
  e := FindByEditorID('STRE_NPC_MessireValen');
  Require(EditorID(LinksTo(ElementByPath(e,'VTCK')))='MaleWarlock','Unexpected Valen voice type');
  e := FindByEditorID('STRE_INFO_VALEN_SELF_010_R');
  Require(GetElementNativeValues(e,'ENAM\Flags')=$A00,'Unexpected trial flags');
  output := TStringList.Create;
  try
    if mode='Prepare' then begin
      try Build; ExportPrivate;
      except on E: Exception do begin
        output.Add('FAIL: '+E.Message); output.SaveToFile(root+'authoring-result.txt'); Exit;
      end; end;
    end;
    output.Add('SUCCESS '+mode);
    output.SaveToFile(root+'authoring-result.txt');
  finally output.Free; end;
end;
end.
