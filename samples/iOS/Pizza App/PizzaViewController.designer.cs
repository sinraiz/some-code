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
	[Register ("PizzaViewController")]
	partial class PizzaViewController
	{
		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UITableView tvPizzaList { get; set; }

		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UINavigationBar uiNavBar { get; set; }

		void ReleaseDesignerOutlets ()
		{
			if (tvPizzaList != null) {
				tvPizzaList.Dispose ();
				tvPizzaList = null;
			}
			if (uiNavBar != null) {
				uiNavBar.Dispose ();
				uiNavBar = null;
			}
		}
	}
}
