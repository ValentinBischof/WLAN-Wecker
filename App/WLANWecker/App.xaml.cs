﻿using System;
using WLANWecker.View;
using Xamarin.Forms;
using Xamarin.Forms.Xaml;

namespace WLANWecker
{
    public partial class App : Application
    {
        public App()
        {
            InitializeComponent();

            MainPage = new AppShell();
        }

        protected override void OnStart()
        {
        }

        protected override void OnSleep()
        {
        }

        protected override void OnResume()
        {
        }
    }
}
