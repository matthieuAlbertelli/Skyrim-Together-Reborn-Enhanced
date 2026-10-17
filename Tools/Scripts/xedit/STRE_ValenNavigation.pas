{ Typed navigation upgrade. xEdit owns every allocation and serialization. }
unit STRE_ValenNavigation;
var target, e, t: IInterface;
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
  if not Assigned(Result) then Result := MainRecordByEditorID(GroupBySignature(target,'DLBR'),edid);
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
  Require(Assigned(Result),'Missing navigation record: '+edid);
end;

function AppendEntry(obj: IInterface; path: string): IInterface;
var arr: IInterface;
begin
  if not ElementExists(obj,path) then begin
    Add(obj,path,True); arr := ElementByPath(obj,path);
    Require(ElementCount(arr)<=1,'Unexpected initial array');
    if ElementCount(arr)=0 then Result := ElementAssign(arr,HighInteger,nil,False)
    else Result := ElementByIndex(arr,0);
  end else Result := ElementAssign(ElementByPath(obj,path),HighInteger,nil,False);
end;

procedure ClearField(obj: IInterface; path: string);
begin
  if ElementExists(obj,path) then RemoveElement(obj,path);
end;

procedure Link(obj: IInterface; path: string; ref: IInterface);
begin
  Require(Assigned(ref),'Missing link: '+path);
  Add(obj,path,True); SetElementNativeValues(obj,path,GetLoadOrderFormID(ref));
end;

function NewTopic(edid, branch, prompt: string): IInterface;
begin
  Require(not Assigned(MainRecordByEditorID(GroupBySignature(target,'DIAL'),edid)),'Topic exists');
  Result := Add(GroupBySignature(target,'DIAL'),'DIAL',True);
  SetElementEditValues(Result,'EDID',edid);
  Link(Result,'QNAM',FindByEditorID('STRE_QUEST_ValenDialogue'));
  Link(Result,'BNAM',FindByEditorID(branch));
  Add(Result,'FULL',True); SetElementEditValues(Result,'FULL',prompt);
  SetElementNativeValues(Result,'PNAM',50.0); Add(Result,'DATA',True);
  SetElementNativeValues(Result,'DATA\Category',0); SetElementNativeValues(Result,'DATA\Subtype',0);
  Add(Result,'SNAM',True); SetElementNativeValues(Result,'SNAM',$54535543);
  Add(Result,'TIFC',True);
end;

function NewInfo(edid, topic: string): IInterface;
begin
  Result := Add(FindByEditorID(topic),'INFO',True);
  SetElementEditValues(Result,'EDID',edid); Add(Result,'ENAM',True);
end;

procedure MoveInfo(info, topic: IInterface);
var oldGroup, newGroup, moved: IInterface; formId: cardinal;
begin
  oldGroup := GetContainer(info); newGroup := ChildGroup(topic);
  if Equals(oldGroup,newGroup) then Exit;
  Require(Assigned(newGroup),'Destination INFO group missing');
  formId := GetLoadOrderFormID(info);
  moved := RemoveByIndex(oldGroup,IndexOf(oldGroup,info),True);
  InsertElement(newGroup,ElementCount(newGroup),moved);
  Require(GetLoadOrderFormID(info)=formId,'Moved INFO identity changed');
  Require(Equals(GetContainer(info),newGroup),'Moved INFO parent mismatch');
end;

procedure AddResponse(obj: IInterface; index: integer; value: string);
var r: IInterface;
begin
  r := AppendEntry(obj,'Responses');
  SetElementNativeValues(r,'TRDT\Response number',index);
  SetElementEditValues(r,'NAM1',value);
end;

procedure AddTopicLink(obj: IInterface; edid: string);
begin
  SetNativeValue(AppendEntry(obj,'Link To'),GetLoadOrderFormID(FindByEditorID(edid)));
end;

procedure AddCondition(obj: IInterface; func, ref: string; value: float; op: integer);
var c: IInterface;
begin
  c := AppendEntry(obj,'Conditions');
  SetElementNativeValues(c,'CTDA\Type',op);
  SetElementEditValues(c,'CTDA\Function',func);
  SetElementNativeValues(c,'CTDA\Comparison Value - Float',value);
  SetElementEditValues(c,'CTDA\Run On','Subject');
  if func='GetIsID' then SetElementNativeValues(c,'CTDA\Referenceable Object',GetLoadOrderFormID(FindByEditorID(ref)))
  else SetElementNativeValues(c,'CTDA\Quest',GetLoadOrderFormID(FindByEditorID(ref)));
end;

procedure Build;
begin
  { GENERATED_NAVIGATION }
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
    config.LoadFromFile(root+'authoring-mode.txt'); Require(config.Count=2,'Invalid wrapper marker');
    Require(SameText(config[0],DataPath),'Marker/Data mismatch'); mode := config[1];
    Require((mode='DryRun') or (mode='Prepare'),'Invalid mode');
  finally config.Free; end;
  for i := 0 to FileCount-1 do
    if GetFileName(FileByIndex(i))='STRE_AlternateStart.esp' then target := FileByIndex(i);
  Require(Assigned(target),'Missing private plugin'); Require(MasterCount(target)=3,'Unexpected masters');
  Require(GetFileName(MasterByIndex(target,0))='Skyrim.esm','Master order');
  Require(GetFileName(MasterByIndex(target,1))='Update.esm','Master order');
  Require(GetFileName(MasterByIndex(target,2))='Dragonborn.esm','Master order');
  e := FindByEditorID('STRE_NPC_MessireValen');
  Require(EditorID(LinksTo(ElementByPath(e,'VTCK')))='MaleWarlock','Valen voice type');
  output := TStringList.Create;
  try
    if mode='Prepare' then begin
      try Build; ExportPrivate;
      except on E: Exception do begin
        output.Add('FAIL: '+E.Message); output.SaveToFile(root+'authoring-result.txt'); Exit;
      end; end;
    end;
    output.Add('SUCCESS '+mode); output.SaveToFile(root+'authoring-result.txt');
  finally output.Free; end;
end;
end.
