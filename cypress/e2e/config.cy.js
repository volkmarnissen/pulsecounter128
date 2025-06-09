
var validateJson = (json)=>{

}
function backspaces(num) {
  let rc = ''
  for (let a = 0; a < num; a++) rc = rc + '{backspace}'
  return rc
}
function triggerSubmit(){
  cy.get('form').submit()
  cy.wait('@postConfig')
}
describe('template spec', () => {
  beforeEach(()=>{
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
      validateJson(req.body)
      req.reply({
        statusCode: 201,
        body: {}
      })
    }).as('postConfig');
    cy.visit('./lib/common/html/index.html')
  })
 
  
 
  it('Basic functionality', () => {
    // wait for loading from server
    validateJson = function(json){
      expect(json.counters.length).to.equal(1)
      expect(json.counters[0].divider).to.equal(888777)
    }
    cy.get("button[id='out_0']").click().get('input[name="divider_0_0"').type("777").then(()=>{
        cy.get('input[name="hour"').should('have.value', "23");
        cy.get('input[name="minute"').should('have.value', "50-54");
        cy.get('input[name="second"').should('have.value', "12,50-53");
        triggerSubmit()
     })
  })
   it('Schedule: *,"",7', () => {
        validateJson = function(json){
          expect(json.schedule.hour.length).to.equal(24) 
          expect(json.schedule.minute.length).to.equal(0) 
          expect(json.schedule.second.length).to.equal(1) 
          expect(json.schedule.second[0]).to.equal(7)
         }
         cy.get("button[id='out_0']").click()
          .get('input[name="hour"').type(backspaces(10) + "*")
          .get('input[name="minute"').type(backspaces(10))
          .get('input[name="second"').type(backspaces(10) + "7")
          .then(()=>{
            triggerSubmit()
          })
   })
   it('Schedule: 5-9', () => {
        validateJson = function(json){
          expect(json.schedule.hour.length).to.equal(5) 
          expect(json.schedule.hour[0]).to.equal(5)
          expect(json.schedule.hour[4]).to.equal(9)
         }
         cy.get("button[id='out_0']").click()
          .get('input[name="hour"').type(backspaces(10) + "5-9")

          .then(()=>{
            triggerSubmit()
          })
   })
}) 