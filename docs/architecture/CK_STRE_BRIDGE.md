# Bridge Creation Kit / Papyrus ↔ STRE

> **Statut : Spécification proposée**

## But

Offrir au contenu CK un petit port stable permettant d’émettre des intentions et de recevoir des résultats sans faire de Papyrus l’autorité réseau.

## Règle de flux

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

Événements :

```papyrus
Event OnSTREAdapterReady(String adapterId, Int version)
Event OnSTRECanonicalEvent(String adapterId, String capability, Int version, String payloadJson)
Event OnSTREIntentRejected(String requestId, String errorCode, String details)
Event OnSTRESnapshotApplied(String adapterId, Int version)
```

Le JSON est utile pour prototyper, mais une représentation plus compacte pourra être nécessaire. Les payloads doivent rester bornés et validés.

## Mode solo

`IsAvailable()` retourne faux. Le mod exécute son flux solo. Les scripts ne doivent jamais attendre indéfiniment un callback STRE absent.

Patron recommandé :

```papyrus
If STREBridge.IsAvailable()
    STREBridge.SubmitIntent(...)
Else
    ApplySoloTransition()
EndIf
```

## Threading et cadence

- Les callbacks Papyrus sont planifiés sur le contexte sûr du jeu.
- Aucune lecture fréquente par polling à chaque frame.
- Les observations sont événementielles ou périodiques à cadence limitée.
- Les appels réseau ne bloquent jamais Papyrus.

## Erreurs

Codes minimaux :

- `ADAPTER_UNAVAILABLE`
- `CAPABILITY_UNSUPPORTED`
- `INVALID_PAYLOAD`
- `NOT_AUTHORIZED`
- `STALE_VERSION`
- `INVALID_PHASE`
- `PLAYER_NOT_IN_CAMPAIGN`
- `RATE_LIMITED`
- `SERVER_UNAVAILABLE`

## Références CK

- propriétés de scripts pour Form/Quest/Global ;
- Editor IDs préfixés `STRE_` ;
- aucun FormID codé en dur ;
- les objets de conséquence doivent être idempotents ;
- les stages de quête locaux sont des rendus de l’état canonique, pas sa source.
