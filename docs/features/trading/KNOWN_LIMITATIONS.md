# Trading — Limites connues

> **Statut : Constaté / à suivre**

## Fonctionnelles

- pas d’or ;
- pas d’objets de quête ;
- pas d’objets équipés ;
- pas d’instances complexes ou enchantées ;
- pas de split d’instances hétérogènes ;
- une seule session par joueur ;
- pas de reprise après restart serveur.

## Architecture

- `IsMvpTransferable` est dupliqué client/serveur ;
- l’UI native utilise un multiplexage sur `toggleDebugUI` ;
- l’inventaire client reste une frontière fragile vis-à-vis du moteur ;
- la véritable atomicité est remplacée par saga + réconciliation ;
- les tests sont surtout unitaires, pas end-to-end réseau.

## UX

- messages d’erreur encore techniques ;
- absence de représentation explicite du délai d’invitation ;
- navigation manette à valider ;
- accessibilité et localisation incomplètes ;
- comportement sur ultra-wide à tester.
