# Valen — Prototype script

> **Status: Prototype; rewrite after the narrative bible is approved**

## Convention

Every final line must have an ID, scene, intent, emotion, condition, audio file, and status.

The spoken lines below are French player-facing localization. Their surrounding production specification is canonical English documentation.

## Scene 1 — The guests

### `VALEN_INTRO_001`

- **Intent:** gain attention.
- **Emotion:** calm, slightly tense.
- **French localized prototype text:** « Vous êtes enfin là. Tous, à ce que je vois. »

### `VALEN_INTRO_002`

- **Intent:** acknowledge the omission.
- **Emotion:** sincere discomfort.
- **French localized prototype text:** « Je vous dois d’abord des excuses. Chacun de vous a reçu une invitation qui ne mentionnait personne d’autre. C’était délibéré. Ce n’était peut-être pas judicieux. »

### `VALEN_INTRO_003`

- **Intent:** establish that he knows them by reputation.
- **Emotion:** precise.
- **French localized prototype text:** « Je ne connais aucun de vous. Pas véritablement. Je connais des récits : une route où quelqu’un aurait survécu, un témoin qui jure avoir vu l’impossible, une dette réglée là où personne n’attendait de secours. »

### `VALEN_INTRO_004`

- **Intent:** present the hypothesis.
- **Emotion:** restrained conviction.
- **French localized prototype text:** « Pris séparément, ces récits ne prouvent rien. Ensemble, ils dessinent une possibilité que je ne peux plus ignorer. L’un de vous pourrait être le Dovahkiin. »

### `VALEN_INTRO_005`

- **Intent:** defuse an immediate election.
- **Emotion:** firm.
- **French localized prototype text:** « Non, je ne sais pas lequel. Et je me méfierais de quiconque prétendrait le savoir déjà. »

### `VALEN_INTRO_006`

- **Intent:** refocus attention on the company.
- **Emotion:** persuasive.
- **French localized prototype text:** « Une prophétie ne traverse pas Skyrim seule. Même si l’un de vous porte ce destin, il aura besoin des autres pour survivre assez longtemps afin de l’accomplir. »

### `VALEN_INTRO_007`

- **Intent:** begin preparation.
- **Emotion:** practical.
- **French localized prototype text:** « Parlez. Équipez-vous. Décidez de ce que chacun peut apporter aux autres. Lorsque vous serez prêts, nous parlerons de la route. »

## Required variants

- one player in single-player;
- 2–4 players;
- 5–10 players;
- collective scene restore after a roster disconnect;
- interruption or departure;
- checkpoint-safe replay/resumption without duplicating canonical progression;
- skeptical, hostile, or curious responses without multiplying canonical branches.

## Audio production

Name: `STRE_Valen_<Scene>_<LineId>_<Take>.wav`

Example: `STRE_Valen_Intro_001_T01.wav`.
