# Invented navigation fixtures

No Bethesda records or binary payloads. `before.esp` is the existing synthetic
matrix fixture projected through the test-only typography helper (variant A).
`authored.esp` is the official xEdit 4.1.5f navigation export from that baseline.

- Before SHA256: `4D1CA7FFC6015D2193F73FAE84D5A08FE737B499DD53DDA323B18753F1ADE93B`
- Authored SHA256: `4131420A8C83A4AE9A96AEB6DD5A82825E76CFC705D0C67CAC8B44ADC93211B5`
- Reproduced by `Test-StreValenNavigationAuthor.ps1` in private session
  `fe44f7c729fc45f2ac2f680a6e8eda31`: DryRun and Prepare PASS, originals unchanged,
  independent topology audit PASS. 307 synthetic records, four additions.

The first session `411445b92cf34c3cb1b45f2f071ae3d4` exported identical bytes but
its initial audit failed because the new reader incorrectly assumed a two-byte
ENAM. The reader was corrected to the actual flags/reset-hours four-byte layout
and the entire integration was rerun. This is serialization evidence only,
not a human xEdit attestation or an engine navigation PASS.
