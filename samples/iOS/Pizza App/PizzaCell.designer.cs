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
	[Register ("PizzaCell")]
	partial class PizzaCell
	{

		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UIImageView imBtnTest { get; set; }

		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UILabel lblOrdersCount { get; set; }

		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UILabel lblPizzaTitle { get; set; }
        

		void ReleaseDesignerOutlets ()
		{
			if (imBtnTest != null) {
				imBtnTest.Dispose ();
				imBtnTest = null;
			}
			if (lblOrdersCount != null) {
				lblOrdersCount.Dispose ();
				lblOrdersCount = null;
			}
			if (lblPizzaTitle != null) {
				lblPizzaTitle.Dispose ();
				lblPizzaTitle = null;
			}
		}
	}
}
