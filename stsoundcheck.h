inline int RunSoundCheck()
{
    int failures=0;
    auto check=[&](bool ok,const char *name) {
        std::printf("soundcheck: %s: %s\n",name,ok?"OK":"FAIL");
        if(!ok) ++failures;
    };
    snd::Mixer mix;
    auto a=std::make_shared<snd::Clip>();a->pcm={100,200,300};
    auto b=std::make_shared<snd::Clip>();b->pcm={10,20,30,40,50};
    auto first=mix.startTracked(a),second=mix.startTracked(b);
    check(first && second && first!=second,"simultaneous playback instances have different handles");
    std::vector<int16_t> pcm(2);mix.render(pcm);
    check(pcm==std::vector<int16_t>({110,220}),"independent voices mix their PCM samples");
    mix.stop(first);mix.render(pcm);
    check(pcm==std::vector<int16_t>({30,40}) && mix.active(second) && !mix.active(first),
          "stopping one object preserves the other voice and its playback position");
    mix.render(pcm);
    check(pcm==std::vector<int16_t>({50,0}) && !mix.active(second),"one-shot completion invalidates its handle");
    auto replacement=mix.startTracked(a);mix.stop(first);mix.render(pcm);
    check(mix.active(replacement) && pcm==std::vector<int16_t>({100,200}),
          "stale handle cannot stop a later sound in the reused slot");
    mix.silence();auto loop=mix.startTracked(a,0.5f,true);pcm.resize(8);mix.render(pcm);
    check(pcm==std::vector<int16_t>({50,100,150,50,100,150,50,100}) && mix.active(loop),
          "tracked loop wraps inside a PCM buffer with the requested gain");
    mix.stop(loop);mix.render(pcm);
    check(pcm==std::vector<int16_t>(8,0),"stopping a loop produces silence in subsequent buffers");
    auto stale=mix.startTracked(a);
    for(int i=0;i<snd::Mixer::VOICES;++i) mix.startTracked(b);
    int voices=mix.voices();mix.stop(stale);
    check(!mix.active(stale) && voices==snd::Mixer::VOICES && mix.voices()==voices,
          "voice eviction invalidates its old handle without affecting the replacement");
    mix.silence();auto loud=std::make_shared<snd::Clip>();loud->pcm={30000,-30000};
    mix.startTracked(loud);mix.startTracked(loud);pcm.resize(2);mix.render(pcm);
    check(pcm==std::vector<int16_t>({32767,-32768}),"PCM mixing clips both signs without integer wraparound");
    check(!mix.startTracked(nullptr) && !mix.startTracked(std::make_shared<snd::Clip>()),
          "invalid clips never allocate a handle");
    auto closing=mix.startTracked(a,1,true);mix.close();
    check(!mix.active(closing) && mix.voices()==0,"closing clears voices even without an audio device");
    auto &m=g_menu;m.blds.clear();m.units.clear();m.prod.clear();m.me=0;
    Menu::Bld building;building.owner=0;building.tobj=50;building.hp=100;building.hpMax=2000;
    building.repairing=true;building.repairSound=m.sfx.mix.startTracked(a,1,true);
    auto other=m.sfx.mix.startTracked(b,1,true);m.blds.push_back(building);
    m.sel.clear();m.selBld={0};m.players[0].civ=0;
    m.CmdAction(true,0);
    check(!m.sfx.mix.active(building.repairSound) && m.sfx.mix.active(other),
          "cancel repair stops only that building's tracked audio");
    m.blds[0]=building;m.blds[0].repairSound=m.sfx.mix.startTracked(a,1,true);
    auto destroyed=m.blds[0].repairSound;m.blds[0].hp=0;m.Reap();
    check(!m.sfx.mix.active(destroyed) && m.sfx.mix.active(other),
          "reaping a destroyed building stops its audio without silencing other objects");
    m.blds.push_back(building);m.blds[0].repairSound=m.sfx.mix.startTracked(a,1,true);
    auto finished=m.blds[0].repairSound;m.blds[0].repairing=false;m.StepRepairVisual(0);
    check(!m.sfx.mix.active(finished) && m.blds[0].repairSound==0,
          "finished repair releases its object sound channel");
    m.blds.assign(2,building);
    m.blds[0].repairSound=m.sfx.mix.startTracked(a,1,true);
    m.blds[1].repairSound=m.sfx.mix.startTracked(b,1,true);
    auto survivor=m.blds[1].repairSound;m.blds[0].hp=0;m.Reap();
    check(m.blds.size()==1 && m.blds[0].repairSound==survivor && m.sfx.mix.active(survivor),
          "building compaction preserves the surviving building's playback handle");
    std::vector<uint8_t> repairWav;
    check(m.sfx.wrap(m.sfx.resolve(867),repairWav) && repairWav.size()>44,
          "original repair sound 867 resolves to an available recording");
    m.sfx.mix.silence();
    std::printf("soundcheck: %d failures\n",failures);
    return failures?1:0;
}
