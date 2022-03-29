using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Net.NetworkInformation;
using System.Net.Sockets;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Input;
using WLANWecker.Helper;
//using Makaretu.Dns;
using WLANWecker.Model;
using WLANWecker.View;
using Xamarin.Forms;
using Zeroconf;

namespace WLANWecker.ViewModel
{
    public class ClockItemViewModel : BindableObject
    {
        public ICommand SearchClocks { get; }
        public ObservableCollection<ClockItem> ClockItems { get; }

        public bool noItems { get; private set; }
        public bool isSearching { get; private set; }

        private string baseDomain = "http://wifi-clock-";
        private string endDomain = ".local/";
        private string staticIP = "192.168.1.184"; 

        private ClockItem selectedItem;
        public ClockItem SelectedItem
        {
            get { return null; } 
            set
            {
                ItemWasSelected(value);
                selectedItem = value; 
            }
        }

        public ClockItemViewModel()
        {
            SearchClocks = new Command(SearchForClocks);
            ClockItems = new ObservableCollection<ClockItem>();

            SearchForClocks();
        }

        private async void CallAlert(ClockItem item)
        {
            await Application.Current.MainPage.DisplayAlert(item.displayName, item.clockDescription, "Ok");
        }

        public async void SearchForClocks()
        {

            ClockItems.Clear(); // Lösche alle alten Wecker Einträge
            noItems = false; // "Kein Geräte" Text soll deaktiviert sein
            isSearching = true; // Sucht nach neuen Geräten

            OnPropertyChanged("noItems");
            OnPropertyChanged("isSearching");

            try // Fehlermeldung: FM01
            {
                IReadOnlyList<IZeroconfHost> responses = null;
                responses = await ZeroconfResolver.ResolveAsync("_http._tcp.local.", scanTime: TimeSpan.FromSeconds(4), retries: 4);

                foreach (var resp in responses)
                {
                    if (resp.DisplayName.Contains("wifi-clock"))
                    {
                        ClockItems.Add(new ClockItem
                        {
                            clockDescription = $"Dies ist dein {resp.DisplayName} Wecker, nicht schlecht oder?",
                            displayName = resp.DisplayName,
                            name = "internalName",
                            ip = resp.IPAddress

                        });
                    }       
                }

                if (await Utilities.CheckStatus(staticIP))
                {
                    ClockItems.Add(new ClockItem
                    {
                        displayName = "Neuer Wecker",
                        clockDescription = "Kein Netzwerk gefunden",
                        name = "internalName",
                        ip = staticIP
                    });
                }

            }
            catch (Exception e)
            {
                Console.WriteLine(e.Message);
                await Application.Current.MainPage.DisplayAlert("Ew", "Uhm irgendwas ist passiert das nicht hätte passieren sollen????  Code: FM01 ", "Ok");
            }
            

            if (ClockItems.Count.Equals(0))
                noItems = true;

            isSearching = false;

            OnPropertyChanged("noItems");
            OnPropertyChanged("isSearching");
            OnPropertyChanged(nameof(ClockItems));
        }

        private async void ItemWasSelected(ClockItem item)
        {
            OnPropertyChanged(nameof(SelectedItem));
            
            await Shell.Current.GoToAsync($"{nameof(ClockView)}?ip={item.ip}");
        }
    }  
}
