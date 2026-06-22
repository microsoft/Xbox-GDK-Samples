//-----------------------------------------------------------------------------
// GlobalSuppressions.cs - SolutionUpdater
//
// Advanced Technology Group (ATG)
// Copyright (C) Microsoft Corporation. All rights reserved.
//-----------------------------------------------------------------------------

using System.Diagnostics.CodeAnalysis;

// Assembly-level suppressions - this is an internal developer tool
[assembly: SuppressMessage("Microsoft.Design", "CA2210:AssembliesShouldHaveValidStrongNames")]
[assembly: SuppressMessage("Microsoft.Design", "CA1014:MarkAssembliesWithClsCompliant")]
[assembly: SuppressMessage("Microsoft.Performance", "CA1824:MarkAssembliesWithNeutralResourcesLanguage")]

// CA1303: Do not pass literals as localized parameters - this is an internal tool, localization not required
[assembly: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope = "member", Target = "SolutionUpdater.MainWindow.#InitializeComponent()")]
[assembly: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope = "member", Target = "SolutionUpdater.MainWindow.#ProcessSolutionFile(System.String,System.Boolean):System.Boolean")]
[assembly: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope = "member", Target = "SolutionUpdater.MainWindow.#ProcessProjectFile(System.String,System.Collections.Generic.Dictionary`2<System.String,SolutionUpdater.MainWindow+FoundProjectInfo>&,System.Boolean):System.Int32")]
[assembly: SuppressMessage("Microsoft.Globalization", "CA1303:DoNotPassLiteralsAsLocalizedParameters", Scope = "member", Target = "SolutionUpdater.MainWindow.#StartUpdate_Click(System.Object,System.EventArgs)")]

// CA2204: Literals should be spelled correctly - tool-specific terminology is intentional
[assembly: SuppressMessage("Microsoft.Naming", "CA2204:LiteralsShouldBeSpelledCorrectly", Scope = "member", Target = "SolutionUpdater.MainWindow.#InitializeComponent()")]
[assembly: SuppressMessage("Microsoft.Naming", "CA2204:LiteralsShouldBeSpelledCorrectly", Scope = "member", Target = "SolutionUpdater.MainWindow.#ProcessSolutionFile(System.String,System.Boolean):System.Boolean")]
[assembly: SuppressMessage("Microsoft.Naming", "CA2204:LiteralsShouldBeSpelledCorrectly", Scope = "member", Target = "SolutionUpdater.MainWindow.#ProcessProjectFile(System.String,System.Collections.Generic.Dictionary`2<System.String,SolutionUpdater.MainWindow+FoundProjectInfo>&,System.Boolean):System.Int32")]
[assembly: SuppressMessage("Microsoft.Naming", "CA2204:LiteralsShouldBeSpelledCorrectly", Scope = "member", Target = "SolutionUpdater.MainWindow.#StartUpdate_Click(System.Object,System.EventArgs)")]

// CA1502: Avoid excessive complexity - these methods necessarily handle multiple platform configurations
[assembly: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "SolutionUpdater.MainWindow.#ProcessSolutionFile(System.String,System.Boolean):System.Boolean")]
[assembly: SuppressMessage("Microsoft.Maintainability", "CA1502:AvoidExcessiveComplexity", Scope = "member", Target = "SolutionUpdater.MainWindow.#ProcessProjectFile(System.String,System.Collections.Generic.Dictionary`2<System.String,SolutionUpdater.MainWindow+FoundProjectInfo>&,System.Boolean):System.Int32")]
