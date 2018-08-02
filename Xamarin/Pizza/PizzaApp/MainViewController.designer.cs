// WARNING
//
// This file has been generated automatically by Xamarin Studio from the outlets and
// actions declared in your storyboard file.
// Manual changes to this file will not be maintained.
//
using Foundation;
using System;
using System.CodeDom.Compiler;
using UIKit;

namespace PizzaApp
{
	[Register ("MainViewController")]
	partial class MainViewController
	{
		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UIBarButtonItem btnExpandAll { get; set; }

		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UIProgressView pbProgress { get; set; }

		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UIView pnlLoadingOverlay { get; set; }

		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UITableView tvPizzaList { get; set; }

		[Action ("BtnExpandAll_Activated:")]
		[GeneratedCode ("iOS Designer", "1.0")]
		partial void BtnExpandAll_Activated (UIBarButtonItem sender);

		void ReleaseDesignerOutlets ()
		{
			if (btnExpandAll != null) {
				btnExpandAll.Dispose ();
				btnExpandAll = null;
			}
			if (pbProgress != null) {
				pbProgress.Dispose ();
				pbProgress = null;
			}
			if (pnlLoadingOverlay != null) {
				pnlLoadingOverlay.Dispose ();
				pnlLoadingOverlay = null;
			}
			if (tvPizzaList != null) {
				tvPizzaList.Dispose ();
				tvPizzaList = null;
			}
		}
	}
}
