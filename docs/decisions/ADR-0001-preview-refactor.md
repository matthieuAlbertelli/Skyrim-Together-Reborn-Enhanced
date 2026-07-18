# ADR-0001 — Refactoring du Preview System

## Contexte
Le code de preview était fortement couplé au service d'échange.

## Décision
Extraire progressivement les responsabilités :
- NativeSession
- Controller
- HostSession
- HostBridge
- RAII Binding

## Conséquences
+ Code plus modulaire
+ Plus facile à tester
+ Prépare les futures évolutions
