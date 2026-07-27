# Bridge Creation Kit / Papyrus ↔ STRE

> **Statut : Intégration first-party partielle implémentée / API générique proposée**

## État actuel

Alternate Start utilise aujourd’hui une intégration dédiée :

```text
Quête CK et TESQuestStageEvent
→ CharacterCreationService natif
→ UI Character Creation
→ application locale ou CharacterBuildRequest
→ résultat canonique appliqué dans Skyrim
```

Le stage `20` de `STRE_QUEST_AlternateStart` déclenche le flux natif/Angular. Les objets, sorts et effets sont authored dans `STRE_AlternateStart.esp`. Le catalogue et le serveur restent la source de vérité des récompenses multijoueur.

Cette intégration valide les principes du bridge, mais **n’expose pas encore** une API Papyrus générique `STREBridge` utilisable par des mods tiers.

## Règle de flux cible

```text
Papyrus observe une interaction
→ envoie une intention au bridge
→ STRE valide et modifie l’état canonique
→ STRE diffuse un événement
→ bridge déclenche un événement Papyrus local
→ le script applique la conséquence visuelle ou Skyrim
```

## API minimale indicative

```papyrus
Bool Function IsAvailable() Global
String Function GetAdapterStatus(String adapterId) Global
Int Function GetCampaignPhase() Global
Bool Function SubmitIntent(String adapterId, String capability, String payloadJson) Global
Int Function GetCanonicalVersion(String adapterId) Global
String Function GetSnapshotJson(String adapterId) Global
```

Événements proposés :

```papyrus
Event OnSTREAdapterReady(String adapterId, Int version)
Event OnSTRECanonicalEvent(String adapterId, String capability, Int version, String payloadJson)
Event OnSTREIntentRejected(String requestId, String errorCode, String details)
Event OnSTRESnapshotApplied(String adapterId, Int version)
```

Le JSON peut servir au prototypage, mais les payloads doivent rester bornés et validés.

## Mode solo

Le plugin Alternate Start doit continuer à fonctionner sans serveur. Le flux M7 utilise le même catalogue localement ; un futur bridge générique devra retourner un état indisponible sans bloquer Papyrus.

Patron cible :

```papyrus
If STREBridge.IsAvailable()
    STREBridge.SubmitIntent(...)
Else
    ApplySoloTransition()
EndIf
```

## Threading et cadence

- callbacks Papyrus planifiés sur le contexte sûr du jeu ;
- aucun polling par frame ;
- observations événementielles ou périodiques limitées ;
- aucun appel réseau bloquant Papyrus.

## Références CK

- EditorID préfixés `STRE_` ;
- propriétés/aliases pour les références de quête ;
- plugin + FormID local stable pour le catalogue natif ;
- aucun préfixe de load order codé en dur ;
- PSC et PEX versionnés tant que la compilation Papyrus n’est pas automatisée ;
- stages de quête locaux considérés comme déclencheurs/projections, pas comme état canonique de campagne.
