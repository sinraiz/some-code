using Foundation;
using PizzaApp.Data;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using UIKit;

namespace PizzaApp
{
    sealed class PizzaTableViewSource : UITableViewSource
    {
        public PizzaTableViewSource(PizzaDataset data)
        {
            _topPizzas = new List<TopPizzaData>();


            /*var random = new Random();

            List<string> allToppings = new List<string>(

                new string []{
                "mozzarella cheese",
                "bacon",
                "beef",
                "onions",
                "pineapple",
                "mushrooms",
                "pepperoni"
            });

            for (int i = 0; i < 200; i++)
            {
                int toppingsCount = 1 + random.Next(allToppings.Count);
                var dataItem = new TopPizzaData()
                {
                    ordersCount = 2 + (uint)random.Next(200)
                };

                allToppings.Shuffle(random);

                for (int j=0; j< toppingsCount; j++)
                {
                    dataItem.toppings.Add(allToppings[j]);
                }

                _topPizzas.Add(dataItem);
            }*/


            foreach (var pizza in data.topData)
            {
                _topPizzas.Add(pizza);
            }
        }

        public override nint NumberOfSections(UITableView tableView)
        {
            return 1;
        }

        public override nint RowsInSection(UITableView tableview, nint section)
        {
            return _topPizzas.Count;
        }

        public override nfloat GetHeightForRow(UITableView tableView, NSIndexPath indexPath)
        {
            var cell = (this.GetCell(tableView, indexPath) as PizzaViewCell);

            // Ask the cell to calculate how high it needs to be
            return cell.fillToppingsAndMeasure(true);
        }

        public override UITableViewCell GetCell(UITableView tableView, NSIndexPath indexPath)
        {
            var cell = (PizzaViewCell)tableView.DequeueReusableCell("PizzaCellID");
            cell.parent = tableView;

            if (indexPath.Row >= _topPizzas.Count)
            {
                cell.data = null;
                cell.title = "error";
                return cell;
            }

            // Set the data
            cell.data = _topPizzas[indexPath.Row];
            cell.title = cell.data.title;
            cell.ordersCount = cell.data.ordersCount;

            // Put the expand/collapse button 
            // into the right state
            if (cell.data.needsRedraw)
            {
                cell.rotateButton(onFinished: () =>
                {
                    cell.data.needsRedraw = false;
                });
            }

            return cell;
        }

        #region Data 
        private readonly List<TopPizzaData> _topPizzas;
        public List<TopPizzaData> topPizzas
        {
            get
            {
                return _topPizzas;
            }
        }
        #endregion
    }

    // TODO: Testing! Remove
    public static class Ext
    {
        public static void Shuffle<T>(this IList<T> list, Random rnd)
        {
            for (var i = 0; i < list.Count; i++)
                list.Swap(i, rnd.Next(i, list.Count));
        }

        public static void Swap<T>(this IList<T> list, int i, int j)
        {
            var temp = list[i];
            list[i] = list[j];
            list[j] = temp;
        }
    }
}
