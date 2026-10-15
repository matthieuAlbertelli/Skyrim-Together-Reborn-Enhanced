# Synthetic authoring evidence

`before.esp` comes entirely from `../fixture.py`: two invented STRE records and
three master names, with no Bethesda record payloads. `authored.esp` is its
official xEdit 4.1.5f `STRE_CreateValenHelgenDialogue.pas` output. These files
exercise the independent auditor, including deliberately corrupted conditions,
parentage, bindings, existing records and French text. They are not game plugins
or human validation evidence. Never deploy them.

Regenerate with `Test-StreValenAuthor.ps1 -XEditPath <official xTESEdit.exe>`;
accept the private module selection and close each completed script window.
Only replace these fixtures after the independent audit succeeds. The fixture
runner verifies its input and all synthetic masters stayed byte-identical.
