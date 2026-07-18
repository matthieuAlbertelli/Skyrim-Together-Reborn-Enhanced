# Architecture - Preview System

## Objectif
Découpler la prévisualisation 3D du système d'échange.

## Architecture

TradeItemPreviewService
    |
    v
ItemPreviewController
    |-- ItemPreviewHostSession
    |-- ItemPreviewNativeSession
    |-- ItemPreviewHostBridge
    |-- ItemPreviewFitSolver
    |-- ItemPreviewRasterMeasurer

## Principes
- Responsabilités isolées
- Refactoring sans changement fonctionnel
- Contrôleur = orchestration
