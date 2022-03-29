using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Net;
using System.Net.Http;
using System.Threading;
using System.Threading.Tasks;
using System.Timers;
using System.Windows.Input;
using WLANWecker.Helper;
using WLANWecker.Model;
using WLANWecker.View;
using Xamarin.Forms;

namespace WLANWecker.ViewModel
{
    [QueryProperty("ip", "ip")]
    public class ClockViewModel : BindableObject
    {
        public bool isSearching { get; private set; }
        public bool isLoadingText { get; private set; }
        public bool isSearchingText { get; private set; }
        public string responseString { get; private set; }

        private System.Timers.Timer timer = new System.Timers.Timer(1000);

        public ICommand AddAlarm { get; }
        public ICommand SaveData { get; }
        public ICommand BackCommand { get; }
        public ICommand SettingsComand { get; }
        public ICommand SettingsView { get; }

        public Clock Clock { get; set; }

        public Alarm selectedItem
        {
            get => null; 
            set
            {
                ItemWasSelected(value);
            } 
        }

        private string _ip;
        public string ip
        {
            get
            {
                return _ip;
            }
            set
            {
                _ip = value;
                SearchForClock(value);
            }
        }

        public ClockViewModel()
        {

            //SaveData = new Command(SaveClock);
            AddAlarm = new Command(AddNewAlarm);
            BackCommand = new Command(ReturnToClockItem);
            SettingsComand = new Command(ShowSettings);
            SettingsView = new Command(ShowSettings);

            timer.Enabled = true;
            timer.Interval = 1000;
            timer.Elapsed += new ElapsedEventHandler(OnTimedEvent);

            //Clock.currentTime = GetCurrentTimeAsString();


        }

        private async void ShowSettings(object obj)
        {
            //await Shell.Current?.GoToAsync(nameof(ClockSettingsView));
            await Shell.Current.GoToAsync($"{nameof(ClockSettingsView)}?ip={ip}&clock={Clock}");
        }

        private void OnTimedEvent(object sender, ElapsedEventArgs e)
        {
            Clock.currentTime = GetCurrentTimeAsString();
            OnPropertyChanged(nameof(Clock));
        }

        public string GetCurrentTimeAsString()
        {
            DateTime time = DateTime.UtcNow.ToLocalTime();

            string hours = time.Hour <= 9 ? $"0{time.Hour}" : $"{time.Hour}";
            string minutes = time.Minute <= 9 ? $"0{time.Minute}" : $"{time.Minute}";
            string seconds = time.Second <= 9 ? $"0{time.Second}" : $"{time.Second}";

            string timeString = $"{hours}:{minutes}:{seconds}";
            return timeString;
        }

        private async void SearchForClock(string ip)
        {
            Clock = new Clock();

            isSearching = true;
            isSearchingText = true;
            Clock.isLoaded = false;
            Clock.currentTime = GetCurrentTimeAsString();

            OnPropertyChanged(nameof(isSearching));
            OnPropertyChanged(nameof(isSearchingText));
            OnPropertyChanged(nameof(Clock));

            if (await Utilities.CheckStatus(ip)) 
            {
                isLoadingText = true;
                isSearchingText = false;

                OnPropertyChanged(nameof(isLoadingText));
                OnPropertyChanged(nameof(isSearchingText));

                Clock = await GetDataFromClock(ip, Clock);

                isLoadingText = false;
                isSearchingText = false;
                isSearching = false;

                OnPropertyChanged(nameof(isLoadingText));
                OnPropertyChanged(nameof(isSearchingText));
                OnPropertyChanged(nameof(isSearching));
                OnPropertyChanged(nameof(responseString));
                OnPropertyChanged(nameof(Clock));
                OnPropertyChanged(nameof(Clock.alarms));
                OnPropertyChanged(nameof(Clock.isLoaded));

                return;
            }

            bool accept = await Application.Current.MainPage.DisplayAlert("Warnung", $"Der Wecker mit der Adresse {ip} wurde " +
                $"nicht gefunden", "Erneut suchen", "Abbrechen");

            if (accept)
            {
                SearchForClock(ip);
            }
            else
            {
                await Shell.Current.GoToAsync("..");
            }
        }

        private async Task<Clock> GetDataFromClock(string ip, Clock clock)
        {
            HttpClient client = new HttpClient(); // Erstelle einen HTTP Client
            string requestUrl = $@"http://{ip}/data/alarms"; // URL zum Abfragen 

            // HTTP Get Request an angegeben URL, speichere antwort zwischen
            HttpResponseMessage response = await client.GetAsync(requestUrl);

            // Lese Antwort als Text aus
            var responseString = await response.Content.ReadAsStringAsync();

            // Konvertiere Text zu Alarm Objekten
            if (responseString.Length != 0)
            {
                if (responseString[responseString.Length - 1] == ';')
                {
                    responseString = responseString.Remove(responseString.Length - 1);
                }

                var alarmsString = responseString.Split(';');

                clock.alarms = LoadAlarmsFromString(alarmsString);
            }

            clock.nightShift = false;
            clock.touchToWake = true;
            clock.isLoaded = true;

            return clock;
        }


        private ObservableCollection<Alarm> LoadAlarmsFromString(string[] alarmArray)
        {
            ObservableCollection<Alarm> alarms = new ObservableCollection<Alarm>();

            foreach (string alarmString in alarmArray)
            {
                Alarm alarm = new Alarm();
                alarm.FromBytes(alarmString);
                alarms.Add(alarm);
            }

            return alarms;
        }

        private async void ItemWasSelected(Alarm item)
        {
            OnPropertyChanged(nameof(selectedItem));
            await Shell.Current.GoToAsync($"{nameof(AlarmView)}?alarm={item.ToBytes()}&ip={ip}");
        }

        private void AddNewAlarm(object obj)
        {
            bool[] ids = new bool[Clock.maxAlarms];
            int openArray = 0;

            if (Clock.alarms != null)
            {
                foreach (Alarm alarm in Clock.alarms)
                {
                    ids[alarm.id] = true;
                }

                for (int i = 0; i < ids.Length; i++)
                {
                    if (ids[i] == false)
                    {
                        openArray = i;
                        break;
                    }
                }
            }
            else
            {
                openArray = 0;
            }

            Alarm newAlarm = new Alarm();
            newAlarm.id = openArray;
            newAlarm.isEnabled = true;
            newAlarm.hour = DateTime.UtcNow.Hour + 1; //Winterzeit
            newAlarm.minute = DateTime.UtcNow.Minute;

            //Clock.alarms.Add(newAlarm);
            ItemWasSelected(newAlarm);
        }


        private async void ReturnToClockItem(object obj)
        {
            await Shell.Current.GoToAsync(nameof(ClockItemView));
        }
    }
}
