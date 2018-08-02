using FFImageLoading.Forms;
using System;
using System.Threading.Tasks;
using Xamarin.Forms;
using TM.Models;
using TM.Navigation;
using TM.Services;

namespace TM.Views
{
    /**
     * This view displays the work records, allows editing and
     * deleteing own records to everybody and also the records
     * from other users based on the current user's role.
     */
    public class WorkPage : BackgroundImageRightNav
    {
        public WorkPage() : base(Utils.FILE("bg1.png"), true)
        {
            Title = "Work Records";
            Icon = Utils.FILE("work.png");

            // Initialize the entire page UI
            setupUI();

            // Setup the filters picker
            setupToolbar();

            // Fetch the work records data
            reloadData();
        }

        /**
         * Setup the UI controls and the layout
         */ 
        private void setupUI()
        {

            //The actual devices list
            lvWork = new WorkListView()
            {
                HorizontalOptions = LayoutOptions.Fill,
                VerticalOptions = LayoutOptions.Fill,
                SeparatorVisibility = SeparatorVisibility.Default,
                SeparatorColor = Color.FromHex("6a6f7b"),
                RowHeight = 60,
                HasUnevenRows = true
            };

            ContentEx = lvWork;
        }
        
        /**
         * Sets up the toolbar buttons: Add and Filter
         */ 
        private void setupToolbar()
        {
            // The toolbar actions
            var mnAdd = new ToolbarItem("Add record", Utils.FILE("add-plus.png"),
                                                   () =>
                                                   {
                                                       addRecord();
                                                   },
                                                   ToolbarItemOrder.Default,
                                                   -1);
            ToolbarItems.Add(mnAdd);

            // The toolbar action
            var mnFilter = new ToolbarItem("Filter", Utils.FILE("filter.png"),
                                                   () =>
                                                   {
                                                       filterSetup();
                                                   },
                                                   ToolbarItemOrder.Default,
                                                   -1);
            ToolbarItems.Add(mnFilter);
        }

        /**
         * Called upon the click on the "filter" button
         * in the page's header.
         * Creates a FilterPage popup, waits for the user
         * to finish with the filters and then applies the
         * updated filter values.
         */ 
        private void filterSetup()
        {
            // Prepare the dialog with this data
            var state = StateController.Current.state;
            var filtersPage = new FiltersPage(state.filtersFrom, state.filtersTo, state.filtersUserId);
            filtersPage.onFiltersApplied += (start, end, userId) =>
            {
                // Retain the updated filters
                state.filtersFrom = start;
                state.filtersTo = end;
                state.filtersUserId = userId;

                // Reload the work records with the new filters
                reloadData();
            };

            // Present the dialog
            App.mainNav.PushAsync(filtersPage, true);
        }

        /**
         * Adds a new work record based on the details provided.
         * Called when the "+" button in the page header is hit.
         * Calls the dialog, waits for the result and tells the 
         * listview to update (add newly created item) 
         */
        private void addRecord()
        {
            // Prepare the new work's record
            var newRecord = new Record();

            // Prepare the dialog with this data
            var workPage = new WorkRecordPage(newRecord);
            workPage.onRecordUpdated += (recAdded) =>
            {
                if (recAdded == null)
                {
                    // something went wrong there
                    return;
                }

                // Inform the list view of a new item
                lvWork.itemAdded(recAdded);
            };

            // Present the dialog
            App.mainNav.PushAsync(workPage, true);
        }

        /**
         * Toggle the list refresh based on the current values
         * of the filters.
         */ 
        private void reloadData()
        {
            var state = StateController.Current.state;
            lvWork.fetchWork(state.filtersFrom, state.filtersTo, state.filtersUserId); ;
        }

        #region UI Controls
        private WorkListView lvWork;
        #endregion
    }

    /**
     * The view that works as a template for every
     * item in the work record's list. 
     * It's bound to Record objects and displays the
     * date, duration etc from those records.
     */ 
    sealed class WorkListCell : ViewCell
    {
        StackLayout layout;
        Label lblWhen;
        Label lblWho;
        Label lblDuration;

        public WorkListCell() : base()
        {
            // Setup actions
            setupRowActions();

            // Add the row contents

            // 1. When the work was done
            lblWhen = new Label
            {
                HorizontalOptions = LayoutOptions.FillAndExpand,
                FontSize = 18,
                BackgroundColor = Utils.clr(App.debugLayout, Color.Pink),
                FontAttributes = FontAttributes.Bold,
                HorizontalTextAlignment = TextAlignment.Start,
                HeightRequest = 25,
            };
            lblWhen.SetBinding(Label.TextProperty, new Binding("when", converter: new DateFormatter()));

            // The user who did the work
            lblWho = new Label
            {
                HorizontalOptions = LayoutOptions.FillAndExpand,
                BackgroundColor = Utils.clr(App.debugLayout, Color.Green),
                FontSize = 16
            };
            lblWho.SetBinding(Label.TextProperty, new Binding("user_name"));

            // The work's duration
            lblDuration = new Label
            {
                HorizontalOptions = LayoutOptions.FillAndExpand,
                FontSize = 25,
                BackgroundColor = Utils.clr(App.debugLayout, Color.Red),
                TextColor = Color.Black,
                VerticalTextAlignment = TextAlignment.Center
            };
            lblDuration.SetBinding(Label.TextProperty, new Binding("duration", converter: new DurationFormatter()));

            // Disclose icon
            var imgArrow = new ContentView
            {
                Padding = new Thickness(3, 0, 3, 0),
                HorizontalOptions = LayoutOptions.FillAndExpand,
                VerticalOptions = LayoutOptions.FillAndExpand,
                Content = new CachedImage
                {
                    Source = "list_arrow.png",
                    HorizontalOptions = LayoutOptions.Center,
                    VerticalOptions = LayoutOptions.Center,
                    HeightRequest = 16,
                    WidthRequest = 16,
                }
            };

            var layout = new Grid()
            {
                BackgroundColor = Color.FromHex("#ebf6fc").MultiplyAlpha(0.5),
                HorizontalOptions = LayoutOptions.FillAndExpand,
                VerticalOptions = LayoutOptions.FillAndExpand,
                RowSpacing = 3,
                ColumnSpacing = 3,
                Padding = new Thickness(5, 5, 0, 5),
                ColumnDefinitions =
                {
                    new ColumnDefinition() {Width = new GridLength(1, GridUnitType.Star) },      // Date and Username
                    new ColumnDefinition() {Width = GridLength.Auto},                            // Duration
                    new ColumnDefinition() {Width = new GridLength(20, GridUnitType.Absolute) }, // Disclose icon
                },
                RowDefinitions =
                {
                    new RowDefinition(){Height = GridLength.Auto},     // Date
                    new RowDefinition(){Height = GridLength.Auto},     // Username
                }
            };

            // Place labels into cells
            layout.Children.Add(lblWhen, 0, 0);
            layout.Children.Add(lblWho, 0, 1);
            layout.Children.Add(lblDuration, 1, 2, 0, 2);
            layout.Children.Add(imgArrow, 2, 3, 0, 2);


            // Finalize the layout
            View = layout;
        }

        protected override void OnBindingContextChanged()
        {
            base.OnBindingContextChanged();

            //The Binding context of this ViewCell to an object type Record
            var pi = (Record)this.BindingContext;
            if(pi == null)
            {
                return;
            }

            // Set the label's color based on if the total worked time is
            // below or above the hours
            lblDuration.TextColor = (pi.isUnderHours) ? Color.Red : Color.Green;

        }

        private void setupRowActions()
        {
            // Define the context actions
            var deleteAction = new Xamarin.Forms.MenuItem
            {
                Text = "Delete",
                IsDestructive = true
            }; // red background

            deleteAction.SetBinding(Xamarin.Forms.MenuItem.CommandParameterProperty, new Binding("id"));
            deleteAction.Clicked += async (sender, e) =>
            {
                var mi = ((Xamarin.Forms.MenuItem)sender);

                var recId = (int)mi.CommandParameter;

                TaskCompletionSource<bool> tcs = new TaskCompletionSource<bool>();

                await Task.Factory.StartNew(async () =>
                {
                    // Call the webservice for vehicle makes list
                    var deleteResult = await WorkService.delete(recId);
                    if (!deleteResult.isOK)
                    {
                        Device.BeginInvokeOnMainThread(async () =>
                        {
                            await App.AppMainWindow.DisplayAlert("Error", "Failed to delete the work record: " + deleteResult.error, "OK");
                        });

                        // Done with waiting
                        tcs.SetResult(false);
                        return;
                    }

                    // Done with waiting
                    tcs.SetResult(true);
                });


                // Wait for the delete operation to finish
                bool result = await tcs.Task;
                if (result)
                {
                    var removedData = this.BindingContext as Record;
                    if (removedData == null)
                    {
                        return;
                    }

                    // Acquire the list context
                    WorkListView lvParent = (removedData.context as WorkListView);

                    // Motify the list of changes
                    lvParent.itemDeleted(removedData);
                }
            };

            // add to the ViewCell's ContextActions property
            ContextActions.Add(deleteAction);
        }
    }

    /**
     * Listview descendant capable of fetching the
     * list of work items from the respective service
     * and displaying them using the WorkListCell
     */
    sealed class WorkListView : ListView
    {
        public WorkListView()
        {
            BackgroundColor = Color.Transparent;
            SeparatorVisibility = SeparatorVisibility.None;
            RowHeight = 50;
            HasUnevenRows = true;

            // Setup the model
            records = new WorkModel();
            ItemsSource = records;

            var cell = new DataTemplate(typeof(WorkListCell));
            ItemTemplate = cell;

            this.ItemSelected += OnItemSelected;
            this.ItemTapped += OnItemTapped;



        }

        public async void fetchWork(DateTime start, DateTime end, int user_id)
        {
            if (isLoading)
                return; // we are already loading something

            // No re-entry
            isLoading = true;

            this.BeginRefresh();

            // Empty the model
            records.Clear();

            // Fetch the data
            var _recordsList = await WorkService.getAll(start, end, user_id);
            if (!_recordsList.isOK)
            {
                await App.AppMainWindow.DisplayAlert("Error", "Failed to get the records list: " + _recordsList.error, "OK");
                return;
            }

            // Place new items into the collection
            foreach (var item in _recordsList.result)
            {
                // Before pushing into model, add ourselves
                // to it for binding puposes
                var boundItem = new Record(item)
                {
                    context = this
                };

                // Add to model
                records.Add(boundItem);
            }

            this.EndRefresh();

            isLoading = false;
        }

        public void itemAdded(Record newRecord)
        {
            // Set the context
            newRecord.context = this;

            // Update the model
            records.add(newRecord);

            // Navigate to the new item
            this.ScrollTo(newRecord, ScrollToPosition.MakeVisible, true);
        }

        public void itemDeleted(Record deletedRecord)
        {
            // Update the model
            records.remove(deletedRecord.id);
        }

        void OnItemTapped(object sender, ItemTappedEventArgs e)
        {
            // Fetch the work record from the row tapped
            var record = e.Item as Record;

            // Prepare the dialog with this data
            var recPage = new WorkRecordPage(record);
            recPage.onRecordUpdated += (rec) =>
            {
                // React by updating the model
                this.BeginRefresh();

                // Let's see what happened to the record

                if (rec.user_id == 0) // it was deleted
                {
                    // Delete based on the user id
                    records.remove(rec.id);

                }
                else // it was updated
                {
                    // Update its data based on the user id
                    records.replace(rec.id, rec);
                }

                this.EndRefresh();
            };

            // Present the dialog
            App.mainNav.PushAsync(recPage, true);
        }

        private void OnItemSelected(object sender, SelectedItemChangedEventArgs e)
        {
            if (e == null) return; // has been set to null, do not 'process' tapped event
            ((ListView)sender).SelectedItem = null; // de-select the row
        }

        // The data object
        private WorkModel records;

        // The data loading flag
        private bool isLoading = false;
    }

    /**
      * This converter is used to format the date 
      * when drawing the databound listview cells
     */ 
    public class DateFormatter : IValueConverter
    {
        #region IValueConverter implementation

        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (value is DateTime)
            {
                DateTime dt = (DateTime)value;
                return dt.ToShortDateString();
            }

            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return value;
        }

        #endregion
    }

    /**
      * This converter is used to format the work
      * record's duration (which is in minutes) as hours 
      * when drawing the databound listview cells
     */
    public class DurationFormatter : IValueConverter
    {
        #region IValueConverter implementation

        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (value is int)
            {
                var duration_int = (int)value;
                double duration = (double)duration_int / 60.0 / 60.0;
                return duration.ToString("n1");
            }

            return value;
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            return value;
        }

        #endregion
    }

}
 