# Better Grabbing multiplayer integration

> **Statut : Implémenté et validé pour le périmètre World Sync actuel**

## Scope

STRE **ne distribue pas** et **ne linke pas** le code de Better Grabbing.

L’utilisateur installe le plugin SKSE séparément. En multijoueur, `BetterGrabbing.dll` est requis par défaut via la politique générique des plugins natifs.

## Responsabilités

### Better Grabbing

- possède l’input local de grab;
- calcule translation/rotation locale;
- applique son comportement local Skyrim;
- continue de fonctionner indépendamment en solo.

### STRE

- détecte les plugins SKSE natifs chargés au handshake;
- applique `ModPolicy:sRequiredNativePlugins`;
- observe le lifecycle via les événements Skyrim disponibles;
- attribue/résout le `WorldEntityId`;
- arbitre l’autorité;
- masque l’objet chez les observateurs pendant le grab;
- gère release, settlement, timeout, disconnect et snapshots;
- gère lazy adoption des références placées;
- préserve l’ownership dans les chemins supportés;
- force une fin de grab si un dialogue Skyrim s’ouvre pendant la manipulation.

STRE ne dépend pas du `Manager` interne de Better Grabbing.

## Native plugin policy

Configuration par défaut :

```ini
[Gameplay]
bEnableItemDrops = true

[ModPolicy]
sRequiredNativePlugins = BetterGrabbing.dll
```

La valeur est une liste de noms de DLL SKSE chargées. Le mécanisme est générique et n’est pas un dependency manager spécifique à Better Grabbing.

## Représentation distante

Pendant un grab accepté :

```text
authority
  Better Grabbing local motion

observer
  local WorldEntity representation hidden
```

Les transforms intermédiaires ne sont pas diffusés pour simuler le mouvement à distance.

Au release :

- l’observateur restaure/repositionne la représentation;
- Havok local reprend;
- le settlement autoritaire final corrige seulement si nécessaire.

## Références placées

La première interaction peut envoyer :

```text
WorldEntityId = 0
PlacedReferenceId = stable GameId de la TESObjectREFR
```

Le serveur résout ou crée atomiquement un `WorldEntityId`.

Chaque client lie ensuite cet ID à sa référence locale existante. **Aucun duplicate spawn**.

Au release distant d’une référence placée, STRE utilise le `MoveTo` existant de STR sur la game thread (`RunnerService`), et non des wrappers `SetPosition`/`SetAngle` spéculatifs.

## Ownership / vol

Si la référence placée a un owner et que Skyrim ne considère pas le joueur comme owner autorisé, le grab déclenche la primitive de vol Skyrim.

La sanction, les témoins et les gardes restent ensuite gérés par les systèmes vanilla.

## Dialogue safety

Le `Dialogue Menu` peut s’ouvrir alors que Better Grabbing tient encore un objet.

STRE force alors la fin du grab par la primitive joueur native. Le `TESGrabReleaseEvent` normal poursuit le lifecycle WorldEntity, ce qui évite :

- contrôles de dialogue bloqués;
- objet restant grabé après arrestation;
- état réseau différent de l’état local.

## Failure recovery

- heartbeat timeout : libération de l’autorité;
- disconnect autorité : release/recovery;
- disconnect observateur : snapshot/rebinding à la reconnexion;
- adoption en attente + release : release différé jusqu’à résolution;
- dialogue pendant grab : forced release local puis lifecycle normal.

## Non-goals

- réimplémenter les contrôles Better Grabbing;
- streamer la physique tenue frame-by-frame;
- redistribuer Better Grabbing;
- dépendre de ses classes internes;
- garantir toutes les configurations Better Grabbing qui modifient fortement collision/physics sans test.
