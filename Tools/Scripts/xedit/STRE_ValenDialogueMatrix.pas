{ Bounded #96 writer. The wrapper inserts only validated typed plan operations.
  xEdit alone allocates and serializes records. Immutable private input only. }
unit STRE_ValenDialogueMatrix;
var target, narrative, dialogue, e, t, b, s, a, v, f: IInterface;
    root, mode, qfName: string;
    output: TStringList;

procedure Require(ok: boolean; msg: string);
begin
  if not ok then raise Exception.Create(msg);
end;

function FindRecord(sig, edid: string): IInterface;
begin
  Result := MainRecordByEditorID(GroupBySignature(target, sig), edid);
end;

function RecordRef(edid: string): IInterface;
begin
  Result := FindRecord('QUST',edid);
  if not Assigned(Result) then Result := FindRecord('NPC_',edid);
  Require(Assigned(Result), 'Unresolved EditorID: '+edid);
end;

function PlannedRecord(sig, edid: string; existing: boolean; parent: IInterface): IInterface;
begin
  if sig = 'INFO' then begin
    Result := MainRecordByEditorID(ChildGroup(parent),edid);
    Require(Assigned(Result)=existing,'Unexpected INFO existence: '+edid);
    if not existing then Result := Add(parent,'INFO',True);
  end else begin
    Result := FindRecord(sig,edid);
    Require(Assigned(Result)=existing, 'Unexpected existence: '+edid);
    if not existing then begin
      if not HasGroup(target,sig) then Add(target,sig,True);
      Result := Add(GroupBySignature(target,sig),sig,True);
    end;
  end;
  SetElementEditValues(Result,'EDID',edid);
end;

function AppendEntry(obj: IInterface; path: string): IInterface;
var arr: IInterface;
begin
  if not ElementExists(obj,path) then begin
    Add(obj,path,True);
    arr := ElementByPath(obj,path);
    Require(ElementCount(arr)<=1,'Unexpected initial array: '+path);
    if ElementCount(arr)=0 then Result := ElementAssign(arr,HighInteger,nil,False)
    else Result := ElementByIndex(arr,0);
  end else begin
    arr := ElementByPath(obj,path);
    { Sorted arrays may insert the blank entry at the beginning. The returned
      interface is the new entry; the last entry may be an existing stage,
      fragment, objective or property and must never be overwritten. }
    Result := ElementAssign(arr,HighInteger,nil,False);
  end;
  Require(Assigned(Result),'Missing array entry: '+path);
end;

procedure ClearField(obj: IInterface; path: string);
begin
  if ElementExists(obj,path) then RemoveElement(obj,path);
end;

procedure Link(obj: IInterface; path: string; ref: IInterface);
begin
  Require(Assigned(ref),'Missing link target: '+path);
  Add(obj,path,True);
  SetElementNativeValues(obj,path,GetLoadOrderFormID(ref));
  Require(Equals(LinksTo(ElementByPath(obj,path)),ref),'Link mismatch: '+path);
end;

procedure AddCondition(obj: IInterface; func, ref: string; value: float; op, stage: integer; variableName: string);
var c: IInterface;
begin
  c := AppendEntry(obj,'Conditions');
  SetElementNativeValues(c,'CTDA\Type',op);
  SetElementEditValues(c,'CTDA\Function',func);
  SetElementNativeValues(c,'CTDA\Comparison Value - Float',value);
  SetElementEditValues(c,'CTDA\Run On','Subject');
  if func='GetIsID' then SetElementNativeValues(c,'CTDA\Referenceable Object',GetLoadOrderFormID(RecordRef(ref)))
  else SetElementNativeValues(c,'CTDA\Quest',GetLoadOrderFormID(RecordRef(ref)));
  if func='GetStageDone' then SetElementNativeValues(c,'CTDA\Quest Stage',stage);
  if func='GetVMQuestVariable' then begin
    SetElementEditValues(c,'CTDA\Variable Name',variableName);
  end;
end;

procedure AddResponse(obj: IInterface; index: integer; text: string);
var r: IInterface;
begin
  r := AppendEntry(obj,'Responses');
  SetElementNativeValues(r,'TRDT\Response number',index);
  SetElementEditValues(r,'NAM1',text);
end;

procedure AddTopicLink(obj: IInterface; edid: string);
var item: IInterface;
begin
  item := AppendEntry(obj,'Link To');
  SetNativeValue(item,GetLoadOrderFormID(FindRecord('DIAL',edid)));
end;

procedure AddStage(quest: IInterface; number: integer; logEntry: boolean);
var stage, arr: IInterface; i: integer;
begin
  arr := ElementByPath(quest,'Stages');
  for i := 0 to ElementCount(arr)-1 do
    if GetElementNativeValues(ElementByIndex(arr,i),'INDX\Stage Index')=number then Exit;
  stage := AppendEntry(quest,'Stages');
  SetElementNativeValues(stage,'INDX\Stage Index',number);
  if logEntry then ElementAssign(stage,1,nil,False);
end;

procedure ObjectProperty(script: IInterface; name: string; ref: IInterface);
var p: IInterface;
begin
  p := AppendEntry(script,'Properties');
  SetElementEditValues(p,'propertyName',name);
  SetElementNativeValues(p,'Type',1);
  SetElementNativeValues(p,'Flags',1);
  SetElementNativeValues(p,'Value\Object Union\Object v2\Alias',-1);
  SetElementNativeValues(p,'Value\Object Union\Object v2\FormID',GetLoadOrderFormID(ref));
end;

procedure ExtendNarrative(objective: string);
var aliasRecord, aliasVmad, fragment, obj, script: IInterface;
begin
  AddStage(narrative,11,True);
  AddStage(narrative,12,True);
  AddStage(narrative,20,True);
  obj := AppendEntry(narrative,'Objectives');
  SetElementNativeValues(obj,'QOBJ',20);
  Add(obj,'FNAM',True);
  SetElementEditValues(obj,'NNAM',objective);
  qfName := GetElementEditValues(narrative,'VMAD\Script Fragments\FileName');
  Require(qfName <> '', 'Missing existing QF');
  fragment := AppendEntry(ElementByPath(narrative,'VMAD\Script Fragments'),'Fragments');
  SetElementNativeValues(fragment,'Quest Stage',20);
  SetElementNativeValues(fragment,'Quest Stage Index',0);
  SetElementEditValues(fragment,'ScriptName',qfName);
  SetElementEditValues(fragment,'FragmentName','Fragment_1');
  SetElementNativeValues(narrative,'ANAM',1);
  aliasRecord := AppendEntry(narrative,'Aliases');
  SetElementNativeValues(aliasRecord,'ALST',0);
  SetElementEditValues(aliasRecord,'ALID','STRE_Valen_LocalPlayer');
  Add(aliasRecord,'FNAM',True);
  Link(aliasRecord,'ALFR',RecordByFormID(MasterByIndex(target,0),$14,True));
  aliasVmad := AppendEntry(ElementByPath(narrative,'VMAD'),'Aliases');
  SetElementNativeValues(aliasVmad,'Object Union\Object v2\Alias',0);
  SetElementNativeValues(aliasVmad,'Object Union\Object v2\FormID',GetLoadOrderFormID(narrative));
  SetElementNativeValues(aliasVmad,'Version',5);
  SetElementNativeValues(aliasVmad,'Object Format',2);
  script := AppendEntry(aliasVmad,'Alias Scripts');
  SetElementEditValues(script,'ScriptName','STRE_ValenHelgenObserver');
  ObjectProperty(script,'Investigation',FindRecord('QUST','STRE_QUEST_HelgenInvestigation'));
  ObjectProperty(script,'HelgenLocation',RecordByFormID(MasterByIndex(target,0),$18A4A,True));
end;

procedure Build;
begin
  narrative := FindRecord('QUST','STRE_QUEST_ValenHelgen');
  dialogue := FindRecord('QUST','STRE_QUEST_ValenDialogue');
  { GENERATED_PLAN }
end;

procedure ExportPrivate(fileName: string);
var stream: TFileStream;
begin
  Require(not FileExists(root+fileName),'Output already exists');
  stream := TFileStream.Create(root+fileName,fmCreate);
  try FileWriteToStream(target,stream,False); finally stream.Free; end;
end;

function Initialize: integer;
var config: TStringList; i: integer;
begin
  Result := 0;
  root := ExtractFileDir(Copy(DataPath,1,Length(DataPath)-1))+'\';
  Require(Pos('\_audit\modding\',LowerCase(root))>0,'Not a private modding session');
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
  Require(Assigned(FindRecord('QUST','STRE_QUEST_ValenHelgen')),'Missing existing narrative');
  Require(not Assigned(FindRecord('DIAL','STRE_DIAL_Valen_Helgen')),'Matrix already authored');
  output := TStringList.Create;
  try
    if mode='Prepare' then begin
      try Build; ExportPrivate('authored.esp');
      except on E: Exception do begin
        output.Add('FAIL: '+E.Message); output.SaveToFile(root+'authoring-result.txt');
        ExportPrivate('rejected.esp'); Exit;
      end; end;
    end;
    output.Add('SUCCESS '+mode);
    output.SaveToFile(root+'authoring-result.txt');
  finally output.Free; end;
end;
end.
