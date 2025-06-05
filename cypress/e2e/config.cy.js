describe('template spec', () => {
  it('passes', () => {
    cy.window()
  .its('console')
  .then((console) => {
    cy.stub(console, 'log').callsFake((...args) => {
      args.forEach((arg) => {
        expect(arg).to.not.contain('error')
      })
      console.log.wrappedMethod(...args)
    })
  })
    cy.intercept("GET", "**/config", {
      fixture: "config.json",
    });
    cy.intercept('POST','**/config', (req) => {
      expect(req.body.counters.length).to.equal(1)
      expect(req.body.counters[0].divider).to.equal(888777)
      req.reply({
        statusCode: 201,
        body: {}
    })
    }).as('postConfig');
    cy.visit('./lib/common/html/index.html')
    // wait for loading from server
    cy.get("button[id='out_0']").click().get('input[name="divider_0_0"').type("777").then(()=>{
      
      cy.get('form').submit()
      cy.wait('@postConfig')
    })
  }
)
}) 