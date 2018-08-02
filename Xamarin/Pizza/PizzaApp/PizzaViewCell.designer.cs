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
	[Register ("PizzaViewCell")]
	partial class PizzaViewCell
	{
		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UIButton btnExpand { get; set; }

		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UILabel lblOrdersCount { get; set; }

		[Outlet]
		[GeneratedCode ("iOS Designer", "1.0")]
		UILabel lblPizzaTitle { get; set; }

		[Action ("BtnExpand_TouchUpInside:")]
		[GeneratedCode ("iOS Designer", "1.0")]
		partial void BtnExpand_TouchUpInside (UIButton sender);

		void ReleaseDesignerOutlets ()
		{
			if (btnExpand != null) {
				btnExpand.Dispose ();
				btnExpand = null;
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
