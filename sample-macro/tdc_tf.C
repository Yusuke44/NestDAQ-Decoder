void tdc_tf(){
  TString fp = "test.root";
  TFile::Open(fp);
  auto tree = gFile->Get<TTree>("tree");
  TCanvas* c1 = new TCanvas("c1","c1");
  c1->Divide(2,2);
  TH1D* h1    = new TH1D("h1","h1",1000,1.8E+11,  2.3E+11);
  TH1D* h2    = new TH1D("h2","FEM Type",11,-0.5,10.5);
  TH1D* h3    = new TH1D("h3","TOT",15,40.5,55.5);
  TH2D* h4    = new TH2D("h4","TDC Corr",1000,1.8E+11,2.3E+11,1000,1.8E+11,2.3E+11);
  
  TTreeReader reader(tree);
  TTreeReaderValue<std::vector<uint64_t>> tdc(reader,"tdc");
  TTreeReaderValue<std::vector<uint64_t>> tot(reader,"tot");
  TTreeReaderValue<std::vector<uint64_t>> ch(reader,"ch");
  TTreeReaderValue<std::vector<uint32_t>> ip_addr(reader,"ip_addr");
  TTreeReaderValue<std::vector<uint8_t>>  ip2(reader,"ip2");
  TTreeReaderValue<std::vector<uint8_t>>  ip3(reader,"ip3");
  TTreeReaderValue<std::vector<uint32_t>> frame_number(reader,"frame_number");
  TTreeReaderValue<std::vector<uint32_t>> fem_type(reader,"fem_type");
  
  while(reader.Next()){
    std::vector<double> tdc_1; //16
    std::vector<double> tdc_2; //161
    int nhits = (*tdc).size();
    for(int i=0; i<nhits; i++){
      if((*ip3)[i]==16 && (*ch)[i]==1){
	uint64_t frame = static_cast<uint64_t>((*frame_number)[i]);
	uint64_t t     = static_cast<uint64_t>((*tdc)[i]);
	if(t==0xffffffffffffffff)continue;
	uint64_t FullTDC = t | (frame << 19);
	double   FullTDC_s = FullTDC * 1.0E-9;
	h1->Fill(FullTDC);
	h2->Fill((*fem_type)[i]);
	h3->Fill((*tot)[i]);
	tdc_1.push_back(FullTDC);
      }
      if((*ip3)[i]==161 && (*ch)[i]==0){
	uint64_t frame = static_cast<uint64_t>((*frame_number)[i]);
	uint64_t t     = static_cast<uint64_t>((*tdc)[i]);
	if(t==0xffffffffffffffff)continue;
	uint64_t FullTDC = t | (frame << 19);
	double   FullTDC_s = FullTDC * 1.0E-9;
	tdc_2.push_back(FullTDC);
      }
    }
    int hit = std::min(tdc_1.size(), tdc_2.size());
    for(int j=0; j<hit; j++){
      h4->Fill(tdc_1[j], tdc_2[j]);
    }
  }

  c1->cd(1);
  h1->Draw();
  h1->GetXaxis()->SetTitle("TDC [ns]");

  c1->cd(2);
  h2->Draw();
  h2->GetXaxis()->SetTitle("FEM Type");

  
  c1->cd(3);
  h3->Draw();
  h3->GetXaxis()->SetTitle("TOT");

  c1->cd(4);
  h4->Draw();
  h4->GetXaxis()->SetTitle("StrHUL");
  h4->GetYaxis()->SetTitle("AMQ");
  gPad->SetFrameFillColor(12);
  gPad->Modified();
  gPad->Update();

}
